/*
  ==============================================================================

    Audio_main.h
    Created: 31 Aug 2026 11:41:57pm
    Author:  Ogiame.xyz

  ==============================================================================
*/

#pragma once

#include <string>
#include <vector>
#include <memory>
#include "Virtual_call.h"

// Forward declaration of internal Objective-C++ / CoreML implementation details
struct CoreMLModelImpl;

namespace faiss {
    class Index; // Forward declaration of C++ FAISS API
}

class AudioMainEngine
{
public:
    AudioMainEngine();
    ~AudioMainEngine();

    // System Initialization & Model Allocation
    bool loadModel(const std::string& mlmodelcPath, const std::string& faissIndexPath);
    void unloadModel();

    // Core Inference Execution Callback called from Audio_module.cpp
    void executeInference(const float* inputFrame2048,
                          float* outputFrame2048,
                          int numSamples,
                          float indexRate,
                          float protect,
                          int pitchShift);

    // Engine Diagnostic Queries
    bool isLoaded() const { return modelLoaded; }
    double getLastInferenceTimeMs() const { return lastExecutionTimeMs; }

private:
    bool modelLoaded = false;
    double lastExecutionTimeMs = 0.0;

    // Pointer to opaque Objective-C++ wrapper holding MLModel instance
    std::unique_ptr<CoreMLModelImpl> coremlImpl;

    // Native C++ FAISS Index for retrieval-based vector search
    faiss::Index* faissIndex = nullptr;

    // Pre-allocated reusable float buffers (prevents memory allocation on audio thread)
    std::vector<float>   hubertFeaturesBuffer;
    std::vector<int64_t> pitchF0Buffer;
    std::vector<float>   modelOutputBuffer;

    // Helper functions for internal feature pipeline steps
    void extractPitchF0(const float* audioInput, int64_t* f0Out, int numSamples, int pitchShift);
    void performIndexSearch(float* embeddings, float indexRate);
};
