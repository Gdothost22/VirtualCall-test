/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "Virtual_call.h"
#include "Audio_main.h"
#include "Audio_module.h"

class VirtualCalltestAudioProcessor : public juce::AudioProcessor
{
public:
    VirtualCalltestAudioProcessor();
    ~VirtualCalltestAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
#endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // Thread-Safe UI Setters & Getters
    void setIndexRate(float rate) { currentParams.indexRate.store(rate, std::memory_order_relaxed); }
    void setProtect(float val)   { currentParams.protectVal.store(val, std::memory_order_relaxed); }
    void setBypass(bool bypass)  { currentParams.bypassEngine.store(bypass, std::memory_order_relaxed); }
    void setPitchShift(int shift){ currentParams.pitchShift.store(shift, std::memory_order_relaxed); }

    double getLastInferenceTimeMs() const { return neuralEngine.getLastInferenceTimeMs(); }
    uint32_t getXRunCount() const { return dspModule.getXRunCount(); }

private:
    // Core Engine Instances (Composition)
    AudioMainEngine neuralEngine;
    AudioModule     dspModule;

    // Local Parameter Cache
    RealtimeParameters currentParams;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VirtualCalltestAudioProcessor)
};