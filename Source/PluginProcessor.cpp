/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

VirtualCalltestAudioProcessor::VirtualCalltestAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                      )
#endif
{
    // 1. Initialize Neural Engine on startup
    juce::File modelFile = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
        .getChildFile("Library/Application Support/VirtualCall/model.mlmodelc");

    juce::File indexFile = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
        .getChildFile("Library/Application Support/VirtualCall/index.index");

    if (!neuralEngine.loadModel(modelFile.getFullPathName().toStdString(),
                                 indexFile.getFullPathName().toStdString()))
    {
        DBG("ERROR: Failed to initialize CoreML Neural Engine or load FAISS Index.");
    }
}

VirtualCalltestAudioProcessor::~VirtualCalltestAudioProcessor()
{
    neuralEngine.unloadModel();
}

const juce::String VirtualCalltestAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool VirtualCalltestAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool VirtualCalltestAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool VirtualCalltestAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double VirtualCalltestAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int VirtualCalltestAudioProcessor::getNumPrograms()
{
    return 1;
}

int VirtualCalltestAudioProcessor::getCurrentProgram()
{
    return 0;
}

void VirtualCalltestAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String VirtualCalltestAudioProcessor::getProgramName (int index)
{
    return {};
}

void VirtualCalltestAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

void VirtualCalltestAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // 2. Prepare lock-free DSP Ring Buffers for hardware block size
    dspModule.prepareToPlay(sampleRate, samplesPerBlock);
}

void VirtualCalltestAudioProcessor::releaseResources()
{
    dspModule.releaseResources();
}

bool VirtualCalltestAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void VirtualCalltestAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // Safety check: Bypass processing if Neural Engine failed to load or bypass toggled on
    if (!neuralEngine.isLoaded() || currentParams.bypassEngine.load(std::memory_order_relaxed))
    {
        return;
    }

    // Update real-time parameters on the fly
    dspModule.setIndexRate(currentParams.indexRate.load(std::memory_order_relaxed));
    dspModule.setProtect(currentParams.protectVal.load(std::memory_order_relaxed));
    dspModule.setPitchShift(currentParams.pitchShift.load(std::memory_order_relaxed));

    // 3. Forward live microphone stream directly to AudioModule (Handles OLA & CoreML thread dispatch)
    dspModule.processStream(buffer, neuralEngine);
}

bool VirtualCalltestAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* VirtualCalltestAudioProcessor::createEditor()
{
    return new VirtualCalltestAudioProcessorEditor (*this);
}

void VirtualCalltestAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
}

void VirtualCalltestAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VirtualCalltestAudioProcessor();
}