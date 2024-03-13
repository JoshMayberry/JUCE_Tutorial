#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
struct CustomRotarySlider : juce::Slider {
    CustomRotarySlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox) { }
};

class SimpleEQAudioProcessorEditor  : public juce::AudioProcessorEditor,
    juce::AudioProcessorParameter::Listener,
    juce::Timer
{
    public:
        SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor&);
        ~SimpleEQAudioProcessorEditor() override;

        //==============================================================================
        void paint (juce::Graphics&) override;
        void resized() override;

        void parameterValueChanged(int parameterIndex, float newValue) override;
        void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {};

        void timerCallback() override;

    private:
        SimpleEQAudioProcessor& audioProcessor;

        juce::Atomic<bool> parametersChanged { false };

        CustomRotarySlider peakFreqSlider, peakGainSlider, peakQualitySlider,
            lowCutFreqSlider, lowCutSlopeSlider, 
            highCutFreqSlider, highCutSlopeSlider;

        using APVTS = juce::AudioProcessorValueTreeState;
        using Attachment = APVTS::SliderAttachment;

        Attachment peakFreqAttachment, peakGainAttachment, peakQualityAttachment,
            lowCutFreqAttachment, lowCutSlopeAttachment,
            highCutFreqAttachment, highCutSlopeAttachment;

        std::vector<juce::Component*> getComps();

        MonoChain monoChain;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessorEditor)
};
