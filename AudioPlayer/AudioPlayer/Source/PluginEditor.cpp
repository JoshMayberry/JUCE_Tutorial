#include "PluginProcessor.h"
#include "PluginEditor.h"

AudioPlayerAudioProcessorEditor::AudioPlayerAudioProcessorEditor(AudioPlayerAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Set the size of the plugin window
    setSize(400, 300);

    // Configure the buttons and add them to the editor
    openButton.setButtonText("Open");
    openButton.addListener(this);
    addAndMakeVisible(&openButton);

    playButton.setButtonText("Play");
    playButton.addListener(this);
    addAndMakeVisible(&playButton);

    stopButton.setButtonText("Stop");
    stopButton.addListener(this);
    addAndMakeVisible(&stopButton);
}

AudioPlayerAudioProcessorEditor::~AudioPlayerAudioProcessorEditor()
{
    // Remove listeners
    openButton.removeListener(this);
    playButton.removeListener(this);
    stopButton.removeListener(this);
}

void AudioPlayerAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    // Additional painting code...
}

void AudioPlayerAudioProcessorEditor::resized()
{
    // Set the position and size of the buttons
    openButton.setBounds(10, getHeight() - 30, getWidth() - 20, 20);
    playButton.setBounds(10, getHeight() - 60, getWidth() - 20, 20);
    stopButton.setBounds(10, getHeight() - 90, getWidth() - 20, 20);
}

void AudioPlayerAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    if (button == &openButton)
    {
        auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        fileChooser.launchAsync(chooserFlags, [this](const juce::FileChooser& chooser)
            {
                auto file = chooser.getResult();
                if (file != juce::File{})  // Check if the user selected a file
                {
                    audioProcessor.loadFile(file);
                }
            });
    }
    else if (button == &playButton)
    {
        audioProcessor.startPlaying();
    }
    else if (button == &stopButton)
    {
        audioProcessor.stopPlaying();
    }
}

