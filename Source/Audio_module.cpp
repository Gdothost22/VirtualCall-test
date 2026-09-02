/*
  ==============================================================================

    Audio_module.cpp
    Created: 31 Aug 2026 6:55:34pm
    Author:  Ogiame.xyz

  ==============================================================================
*/

#include "Audio_module.h"

AudioModule::AudioModule()
    : hannWindow(VirtualCallConfig::kWindowSize, juce::dsp::WindowingFunction<float>::hann, false)
{
    inputRingBuffer.resize(VirtualCallConfig::kRingBufLen, 0.0f);
    outputOLAAccumulator.resize(VirtualCallConfig::kRingBufLen, 0.0f);
    processingWindowBuffer.resize(VirtualCallConfig::kWindowSize, 0.0f);
    inferenceOutputBuffer.resize(VirtualCallConfig::kWindowSize, 0.0f);
}

AudioModule::~AudioModule() {}

void AudioModule::prepareToPlay(double sampleRate, int samplesPerBlockExpected)
{
    std::fill(inputRingBuffer.begin(), inputRingBuffer.end(), 0.0f);
    std::fill(outputOLAAccumulator.begin(), outputOLAAccumulator.end(), 0.0f);
    writeIndex = 0;
    readIndex = 0;
}

void AudioModule::processStream(juce::AudioBuffer<float>& buffer, AudioMainEngine& neuralEngine, RealtimeParameters& params)
{
    if (params.bypassEngine.load(std::memory_order_relaxed))
        return; // Bypass DSP processing if flagged

    const int numSamples = buffer.getNumSamples();
    const float* inRead = buffer.getReadPointer(0); // Mono stream
    float* outWrite = buffer.getWritePointer(0);

    for (int i = 0; i < numSamples; ++i)
    {
        // 1. Push incoming microphone sample to Input Ring Buffer
        int currentWrite = writeIndex.load(std::memory_order_relaxed);
        inputRingBuffer[currentWrite] = inRead[i];

        int nextWrite = (currentWrite + 1) % VirtualCallConfig::kRingBufLen;
        writeIndex.store(nextWrite, std::memory_order_release);

        // 2. Check if a full Hop Size is ready for inference
        int currentRead = readIndex.load(std::memory_order_relaxed);
        int availableSamples = (currentWrite >= currentRead) ? 
            (currentWrite - currentRead) : (VirtualCallConfig::kRingBufLen - currentRead + currentWrite);

        if (availableSamples >= VirtualCallConfig::kHopSize)
        {
            processHopFrame(neuralEngine, params);

            // Step forward by Hop Size
            readIndex.store((currentRead + VirtualCallConfig::kHopSize) % VirtualCallConfig::kRingBufLen, std::memory_order_release);
        }

        // 3. Pull converted Overlap-Add sample out to JUCE speaker output
        outWrite[i] = outputOLAAccumulator[currentRead];

        // 4. Clear processed output accumulator sample to prevent stale noise loops
        outputOLAAccumulator[currentRead] = 0.0f;
    }
}

void AudioModule::processHopFrame(AudioMainEngine& neuralEngine, RealtimeParameters& params)
{
    int startReadIndex = readIndex.load(std::memory_order_relaxed);

    // Load active atomic parameters safely on the audio thread
    float currentIdxRate = params.indexRate.load(std::memory_order_relaxed);
    float currentProtect = params.protectVal.load(std::memory_order_relaxed);
    int   currentPitch   = params.pitchShift.load(std::memory_order_relaxed);

    // 1. Extract Window Size samples from Ring Buffer
    for (int n = 0; n < VirtualCallConfig::kWindowSize; ++n)
    {
        int sampleIdx = (startReadIndex + n) % VirtualCallConfig::kRingBufLen;
        processingWindowBuffer[n] = inputRingBuffer[sampleIdx];
    }

    // 2. Apply Analysis Hann Window
    hannWindow.multiplyWithWindowingTable(processingWindowBuffer.data(), VirtualCallConfig::kWindowSize);

    // 3. Execute Neural Engine Inference (CoreML via AudioMainEngine)
    neuralEngine.executeInference(
        processingWindowBuffer.data(),
        inferenceOutputBuffer.data(),
        VirtualCallConfig::kWindowSize,
        currentIdxRate,
        currentProtect,
        currentPitch
    );

    // 4. Apply Synthesis Window (COLA property)
    hannWindow.multiplyWithWindowingTable(inferenceOutputBuffer.data(), VirtualCallConfig::kWindowSize);

    // 5. Accumulate Overlapping Tail into Output Ring Buffer (Overlap-Add)
    for (int n = 0; n < VirtualCallConfig::kWindowSize; ++n)
    {
        int olaWriteIdx = (startReadIndex + n) % VirtualCallConfig::kRingBufLen;
        outputOLAAccumulator[olaWriteIdx] += inferenceOutputBuffer[n];
    }
}

void AudioModule::releaseResources()
{
    std::fill(inputRingBuffer.begin(), inputRingBuffer.end(), 0.0f);
    std::fill(outputOLAAccumulator.begin(), outputOLAAccumulator.end(), 0.0f);
}
