/*
  ==============================================================================

    Virtual_call.h
    Created: 31 Aug 2026 6:49:40pm
    Author:  Ogiame.xyz

  ==============================================================================
*/

#pragma once

#include <cstdint>
#include <atomic>

// Global Audio Constants matching RVC v2 architecture
namespace VirtualCallConfig
{
    constexpr uint32_t kSampleRate      = 16000; // 16 kHz Mono PCM
    constexpr uint32_t kWindowSize      = 2048;  // N = 128 ms audio context
    constexpr uint32_t kHopSize         = 512;   // H = 32 ms frame step
    constexpr uint32_t kRingBufferLen   = 16384; // Lock-free Ring Buffer size
    constexpr uint32_t kHuBertEmbedding = 768;   // ContentVec feature dimension
}

// Thread-Safe Configuration Struct shared across DSP and UI threads
struct RealtimeParameters
{
    std::atomic<float> indexRate    { 0.50f }; // 0.0 (fast) to 0.6 (accent match)
    std::atomic<float> protectVal   { 0.33f }; // Consonant protection (S/T sounds)
    std::atomic<int>   pitchShift   { -7 };    // -7 semitones default for vocal range shifting
    std::atomic<bool>  bypassEngine { false }; // Dynamic DSP bypass flag
};

// Shared Memory Telemetry Block (Read by UI via POSIX SHM)
struct SharedTelemetryBlock
{
    uint64_t totalFramesProcessed;
    double   lastInferenceLatencyMs; // High-resolution timer execution cost
    uint32_t xRunCount;              // Buffer dropout tracking
    float    inputRmsLevel;
    float    outputRmsLevel;
};
