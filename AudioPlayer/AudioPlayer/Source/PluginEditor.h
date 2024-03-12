#include <JuceHeader.h>
#include "PluginProcessor.h"

class AudioPlayerAudioProcessorEditor : public juce::AudioProcessorEditor,
    public juce::Button::Listener // Inherit from Button::Listener
{
public:
    AudioPlayerAudioProcessorEditor(AudioPlayerAudioProcessor&);
    ~AudioPlayerAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void buttonClicked(juce::Button* button) override; // Implement this method to handle button clicks

private:
    AudioPlayerAudioProcessor& audioProcessor;

    // Add your GUI components here
    juce::TextButton openButton;   // Button to open the audio file
    juce::TextButton playButton;   // Button to play the audio
    juce::TextButton stopButton;   // Button to stop the audio

    juce::FileChooser fileChooser{ "Select a Wave file to play...",
        juce::File{},
        "*.wav,*.mp3" };  // File chooser

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPlayerAudioProcessorEditor)
};
