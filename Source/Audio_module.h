/*
  ==============================================================================

    Audio_module.h
    Created: 31 Aug 2026 6:55:34pm
    Author:  Ogiame.xyz

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <atomic>
#include "Virtual_call.h"

// Forward declaration of your CoreML wrapper in Audio_main.mm
class AudioMainEngine;

class AudioModule
{
public:
    AudioModule();
    ~AudioModule();

    void prepareToPlay(double sampleRate, int samplesPerBlockExpected);
    void processStream(juce::AudioBuffer<float>& buffer, AudioMainEngine& neuralEngine, RealtimeParameters& params);
    void releaseResources();

    // Configuration setters bridging UI to RealtimeParameters
    void setIndexRate(RealtimeParameters& params, float rate)  { params.indexRate.store(rate); }
    void setProtect(RealtimeParameters& params, float value)   { params.protectVal.store(value); }
    void setPitchShift(RealtimeParameters& params, int shift)   { params.pitchShift.store(shift); }

private:
    // Windowing and Buffers using VirtualCallConfig dimensions
    juce::dsp::WindowingFunction<float> hannWindow;

    std::vector<float> inputRingBuffer;
    std::atomic<int>   writeIndex { 0 };
    std::atomic<int>   readIndex  { 0 };

    std::vector<float> outputOLAAccumulator; // Overlap-Add accumulation ring
    std::vector<float> processingWindowBuffer;
    std::vector<float> inferenceOutputBuffer;

    void processHopFrame(AudioMainEngine& neuralEngine, RealtimeParameters& params);
};
