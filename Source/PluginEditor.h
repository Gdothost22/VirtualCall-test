/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class VirtualCalltestAudioProcessorEditor : public juce::AudioProcessorEditor,
                                            private juce::Timer
{
public:
    VirtualCalltestAudioProcessorEditor (VirtualCalltestAudioProcessor&);
    ~VirtualCalltestAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    VirtualCalltestAudioProcessor& audioProcessor;

    // UI Control Sliders & Labels
    juce::Slider indexRateSlider;
    juce::Label  indexRateLabel;

    juce::Slider protectSlider;
    juce::Label  protectLabel;

    juce::ToggleButton bypassButton;

    // Telemetry Diagnostics Panel Labels
    juce::Label latencyValueLabel;
    juce::Label xrunValueLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VirtualCalltestAudioProcessorEditor)
};
