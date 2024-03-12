/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
AudioPlayerAudioProcessor::AudioPlayerAudioProcessor()
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
    formatManager.registerBasicFormats();
}

AudioPlayerAudioProcessor::~AudioPlayerAudioProcessor()
{
}

//==============================================================================
const juce::String AudioPlayerAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPlayerAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool AudioPlayerAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool AudioPlayerAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double AudioPlayerAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPlayerAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int AudioPlayerAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPlayerAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String AudioPlayerAudioProcessor::getProgramName (int index)
{
    return {};
}

void AudioPlayerAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void AudioPlayerAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    transportSource.prepareToPlay(samplesPerBlock, sampleRate);
}

void AudioPlayerAudioProcessor::releaseResources()
{
    transportSource.releaseResources();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool AudioPlayerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void AudioPlayerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());


    // Add this to play the audio file if it's loaded
    if (isFileLoaded)
    {
        transportSource.getNextAudioBlock(juce::AudioSourceChannelInfo(buffer));
    }
    else
    {
        buffer.clear();  // Clear the buffer if no file is loaded to avoid noise
    }
}

//==============================================================================
bool AudioPlayerAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* AudioPlayerAudioProcessor::createEditor()
{
    return new AudioPlayerAudioProcessorEditor (*this);
}

//==============================================================================
void AudioPlayerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void AudioPlayerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPlayerAudioProcessor();
}

void AudioPlayerAudioProcessor::loadFile(const juce::File& audioFile)
{
    auto* reader = formatManager.createReaderFor(audioFile);
    if (reader != nullptr)
    {
        std::unique_ptr<juce::AudioFormatReaderSource> newSource(new juce::AudioFormatReaderSource(reader, true));
        transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
        readerSource.reset(newSource.release());
        isFileLoaded = true;
    }
}

void AudioPlayerAudioProcessor::loadDefaultFile()
{
    if (!isFileLoaded)  // Assuming isFileLoaded is a boolean flag indicating if a file has been loaded
    {
        // Access the binary data and size for your mp3 file
        auto* defaultData = BinaryData::default_song_mp3;  // The identifier might differ, check BinaryData.h
        auto defaultSize = BinaryData::default_song_mp3Size;  // Corresponding size variable

        if (defaultData != nullptr && defaultSize > 0)
        {
            // Create a MemoryInputStream from the binary data
            std::unique_ptr<juce::MemoryInputStream> stream(new juce::MemoryInputStream(defaultData, defaultSize, false));

            // Use an AudioFormatReader to read the stream - assuming formatManager is an AudioFormatManager instance
            if (auto reader = formatManager.createReaderFor(std::move(stream)))
            {
                // Use the reader to load the audio into your playback system
                // (You'll need to adapt this part based on how your system is set up)
                std::unique_ptr<juce::AudioFormatReaderSource> newSource(new juce::AudioFormatReaderSource(reader, true));
                transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
                readerSource.reset(newSource.release());
                isFileLoaded = true;
            }
        }
        else
        {
            DBG("Default audio file could not be loaded.");
        }
    }
}



void AudioPlayerAudioProcessor::startPlaying()
{
    if (!isFileLoaded)
    {
        loadDefaultFile();
    }

    if (isFileLoaded) {
        transportSource.start();
    }
}

void AudioPlayerAudioProcessor::stopPlaying()
{
    transportSource.stop();
}