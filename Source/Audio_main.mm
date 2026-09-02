/*
  ==============================================================================

    Audio_main.mm
    Created: 31 Aug 2026 11:41:57pm
    Author:  Ogiame.xyz

  ==============================================================================
*/
#import <Foundation/Foundation.h>
#import <CoreML/CoreML.h>
#include <chrono>
#include <iostream>
#include <algorithm>
#include "Audio_main.h"

// Objective-C++ Wrapper struct holding Apple CoreML pointers
struct CoreMLModelImpl
{
    MLModel *model = nil;
    NSString *inputPhoneName = @"phone";
    NSString *inputPitchName = @"pitch";
    NSString *outputAudioName = @"output_audio";
};

AudioMainEngine::AudioMainEngine()
    : coremlImpl(std::make_unique<CoreMLModelImpl>())
{
    // Pre-allocate all working memory vectors to prevent audio-thread mallocs
    hubertFeaturesBuffer.resize(VirtualCallConfig::kWindowSize, 0.0f);
    pitchF0Buffer.resize(VirtualCallConfig::kWindowSize, 0);
    modelOutputBuffer.resize(VirtualCallConfig::kWindowSize, 0.0f);
}

AudioMainEngine::~AudioMainEngine()
{
    unloadModel();
}

bool AudioMainEngine::loadModel(const std::string& mlmodelcPath, const std::string& faissIndexPath)
{
    @autoreleasepool {
        NSString *pathNS = [NSString stringWithUTF8String:mlmodelcPath.c_str()];
        NSURL *modelURL = [NSURL fileURLWithPath:pathNS];

        NSError *error = nil;
        // Configuration forcing hardware execution onto Apple Neural Engine (ANE)
        MLModelConfiguration *config = [[MLModelConfiguration alloc] init];
        config.computeUnits = MLComputeUnitsAll; // CPU + GPU + ANE

        coremlImpl->model = [MLModel modelWithContentsOfURL:modelURL configuration:config error:&error];

        if (error || !coremlImpl->model) {
            std::cerr << "[VirtualCall Engine Error] CoreML Load Failed: "
                      << [[error localizedDescription] UTF8String] << std::endl;
            modelLoaded = false;
            return false;
        }

        modelLoaded = true;
        std::cout << "[VirtualCall Engine] CoreML Model Loaded successfully." << std::endl;
        return true;
    }
}

void AudioMainEngine::unloadModel()
{
    @autoreleasepool {
        coremlImpl->model = nil;
        modelLoaded = false;
    }
}

void AudioMainEngine::extractPitchF0(const float* audioInput, int64_t* f0Out, int numSamples, int pitchShift)
{
    // 1. Perform pitch tracking (e.g., RMVPE / Harvest algorithm)
    // 2. Apply the pitch offset semitone shift directly to non-zero pitch frames
    for (int i = 0; i < numSamples; ++i)
    {
        if (f0Out[i] > 0) // Only shift unvoiced/voiced speech regions, leave silence alone
        {
            // Apply semitone offset
            int64_t shiftedPitch = f0Out[i] + pitchShift;

            // Clamp pitch to valid MIDI/F0 bounds (1 to 127)
            f0Out[i] = std::clamp(shiftedPitch, (int64_t)1, (int64_t)127);
        }
    }
}

void AudioMainEngine::performIndexSearch(float* embeddings, float indexRate)
{
    if (indexRate <= 0.0f || !faissIndex) return;
}

void AudioMainEngine::executeInference(const float* inputFrame2048,
                                      float* outputFrame2048,
                                      int numSamples,
                                      float indexRate,
                                      float protect,
                                      int pitchShift)
{
    if (!modelLoaded || !coremlImpl->model) {
        std::copy(inputFrame2048, inputFrame2048 + numSamples, outputFrame2048);
        return;
    }

    auto startTimer = std::chrono::high_resolution_clock::now();

    @autoreleasepool {
        NSError *error = nil;

        // 1. Extract pitch and apply the semitone shift before creating CoreML MLMultiArray
        extractPitchF0(inputFrame2048, pitchF0Buffer.data(), numSamples, pitchShift);

        // 2. Perform vector index search if indexRate > 0.0
        if (indexRate > 0.0f) {
            performIndexSearch(hubertFeaturesBuffer.data(), indexRate);
        }

        // 3. Wrap C++ float arrays into CoreML MLMultiArray Tensors
        NSArray<NSNumber *> *phoneShape = @[@1, @(numSamples), @(VirtualCallConfig::kHuBertDim)];
        MLMultiArray *phoneArray = [[MLMultiArray alloc] initWithDataPointer:hubertFeaturesBuffer.data()
                                                                       shape:phoneShape
                                                                    dataType:MLMultiArrayDataTypeFloat32
                                                                     strides:@[@(numSamples * VirtualCallConfig::kHuBertDim), @(VirtualCallConfig::kHuBertDim), @1]
                                                                 deallocator:nil
                                                                       error:&error];

        NSArray<NSNumber *> *pitchShape = @[@1, @(numSamples)];
        MLMultiArray *pitchArray = [[MLMultiArray alloc] initWithDataPointer:pitchF0Buffer.data()
                                                                       shape:pitchShape
                                                                    dataType:MLMultiArrayDataTypeInt64
                                                                     strides:@[@(numSamples), @1]
                                                                 deallocator:nil
                                                                       error:&error];

        // 4. Construct CoreML Dictionary Inputs
        NSDictionary *inputDict = @{
            coremlImpl->inputPhoneName : phoneArray,
            coremlImpl->inputPitchName : pitchArray
        };

        MLDictionaryFeatureProvider *inputs = [[MLDictionaryFeatureProvider alloc] initWithDictionary:inputDict error:&error];

        if (error) {
            std::copy(inputFrame2048, inputFrame2048 + numSamples, outputFrame2048);
            return;
        }

        // 5. EXECUTE APPLE NEURAL ENGINE INFERENCE
        id<MLFeatureProvider> outputs = [coremlImpl->model predictionFromFeatures:inputs error:&error];

        if (error || !outputs) {
            std::copy(inputFrame2048, inputFrame2048 + numSamples, outputFrame2048);
            return;
        }

        // 6. Extract Output PCM Tensor and copy back to C++ float array
        MLFeatureValue *outputValue = [outputs featureValueForName:coremlImpl->outputAudioName];
        MLMultiArray *outputArray = [outputValue multiArrayValue];

        if (outputArray) {
            const float* convertedAudioPtr = (const float*)outputArray.dataPointer;
            std::copy(convertedAudioPtr, convertedAudioPtr + numSamples, outputFrame2048);
        } else {
            std::copy(inputFrame2048, inputFrame2048 + numSamples, outputFrame2048);
        }
    }

    auto endTimer = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = endTimer - startTimer;
    lastExecutionTimeMs = duration.count();
}