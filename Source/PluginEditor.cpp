/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

VirtualCalltestAudioProcessorEditor::VirtualCalltestAudioProcessorEditor (VirtualCalltestAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // 1. Configure Index Rate Slider (FAISS Accent Retrieval)
    indexRateSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    indexRateSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    indexRateSlider.setRange (0.0, 1.0, 0.05);
    indexRateSlider.setValue (0.50);
    indexRateSlider.onValueChange = [this] {
        audioProcessor.setIndexRate ((float)indexRateSlider.getValue());
    };
    addAndMakeVisible (indexRateSlider);

    indexRateLabel.setText ("Index Rate (Accent)", juce::dontSendNotification);
    indexRateLabel.attachToComponent (&indexRateSlider, false);
    indexRateLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (indexRateLabel);

    // 2. Configure Protect Slider (Consonant S/T Protection)
    protectSlider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    protectSlider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 60, 20);
    protectSlider.setRange (0.0, 0.5, 0.01);
    protectSlider.setValue (0.33);
    protectSlider.onValueChange = [this] {
        audioProcessor.setProtect ((float)protectSlider.getValue());
    };
    addAndMakeVisible (protectSlider);

    protectLabel.setText ("Consonant Protect", juce::dontSendNotification);
    protectLabel.attachToComponent (&protectSlider, false);
    protectLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (protectLabel);

    // 3. Bypass Engine Toggle
    bypassButton.setButtonText ("Bypass CoreML Engine");
    bypassButton.onClick = [this] {
        audioProcessor.setBypass (bypassButton.getToggleState());
    };
    addAndMakeVisible (bypassButton);

    // 4. Telemetry TelePrompter Labels
    latencyValueLabel.setText ("CoreML Latency: 0.00 ms", juce::dontSendNotification);
    latencyValueLabel.setFont (juce::FontOptions(14.0f, juce::Font::bold));
    latencyValueLabel.setColour (juce::Label::textColourId, juce::Colours::cyan);
    addAndMakeVisible (latencyValueLabel);

    xrunValueLabel.setText ("XRuns (Dropouts): 0", juce::dontSendNotification);
    xrunValueLabel.setFont (juce::FontOptions(14.0f, juce::Font::bold));
    xrunValueLabel.setColour (juce::Label::textColourId, juce::Colours::limegreen);
    addAndMakeVisible (xrunValueLabel);

    // Set Window Size & Start Telemetry Timer at 30 FPS (Every 33ms)
    setSize (450, 300);
    startTimer (33);
}

VirtualCalltestAudioProcessorEditor::~VirtualCalltestAudioProcessorEditor()
{
    stopTimer();
}

void VirtualCalltestAudioProcessorEditor::paint (juce::Graphics& g)
{
    // Dark Apple Silicon style aesthetic
    g.fillAll (juce::Colour (0xff1e1e24));

    g.setColour (juce::Colours::white);
    g.setFont (18.0f);
    g.drawFittedText ("Virtual Call Engine Testbench", getLocalBounds().removeFromTop (45), juce::Justification::centred, 1);

    // Diagnostics Frame Box
    g.setColour (juce::Colour (0xff2d2d38));
    g.fillRoundedRectangle (20.0f, 180.0f, 410.0f, 90.0f, 8.0f);
    g.setColour (juce::Colours::grey);
    g.drawRoundedRectangle (20.0f, 180.0f, 410.0f, 90.0f, 8.0f, 1.0f);
}

void VirtualCalltestAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop (45); // Reserve header

    auto controlsArea = bounds.removeFromTop (130);
    indexRateSlider.setBounds (controlsArea.removeFromLeft (180).reduced (10));
    protectSlider.setBounds (controlsArea.removeFromLeft (180).reduced (10));
    bypassButton.setBounds (controlsArea.reduced (10));

    // Telemetry Box Layout
    latencyValueLabel.setBounds (35, 195, 380, 25);
    xrunValueLabel.setBounds (35, 230, 380, 25);
}

void VirtualCalltestAudioProcessorEditor::timerCallback()
{
    // Poll execution telemetry metrics safely from audioProcessor
    double lastLatency = audioProcessor.getLastInferenceTimeMs();
    uint32_t xruns = audioProcessor.getXRunCount();

    latencyValueLabel.setText ("CoreML Execution Latency: " + juce::String (lastLatency, 2) + " ms", juce::dontSendNotification);
    xrunValueLabel.setText ("Buffer Dropouts (XRuns): " + juce::String (xruns), juce::dontSendNotification);

    // Highlight XRuns red if dropouts occur
    if (xruns > 0) {
        xrunValueLabel.setColour (juce::Label::textColourId, juce::Colours::red);
    }
}
