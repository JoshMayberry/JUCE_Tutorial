#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct LookAndFeel : juce::LookAndFeel_V4 {
    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider&) override;
};

//==============================================================================
struct RotarySliderWithLabels : juce::Slider {
    RotarySliderWithLabels(juce::RangedAudioParameter& rap, juce::String unitSuffix) :
        juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox),
        param(&rap),
        suffix(unitSuffix)
    {
        setLookAndFeel(&lnf);
    }

    ~RotarySliderWithLabels() {
        setLookAndFeel(nullptr);
    }

    void paint(juce::Graphics& g) override;
    juce::Rectangle<int> getSliderBounds() const;
    int getTextHeight() const { return 14; };
    juce::String getDisplayString() const;

    private:
        LookAndFeel lnf;

        juce::RangedAudioParameter* param;
        juce::String suffix;
};

struct ResponseCurveComponent : public juce::AudioProcessorEditor,
    juce::AudioProcessorParameter::Listener,
    juce::Timer
{
    public:
        ResponseCurveComponent(SimpleEQAudioProcessor&);
        ~ResponseCurveComponent() override;

        void parameterValueChanged(int parameterIndex, float newValue) override;
        void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {};

        void timerCallback() override;

        void paint(juce::Graphics& g) override;

    private:
        SimpleEQAudioProcessor& audioProcessor;
        juce::Atomic<bool> parametersChanged { false };

        MonoChain monoChain;
};

class SimpleEQAudioProcessorEditor : public juce::AudioProcessorEditor {
    public:
        SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor&);
        ~SimpleEQAudioProcessorEditor() override;

        //==============================================================================
        void paint (juce::Graphics&) override;
        void resized() override;

    private:
        SimpleEQAudioProcessor& audioProcessor;

        RotarySliderWithLabels peakFreqSlider, peakGainSlider, peakQualitySlider,
            lowCutFreqSlider, lowCutSlopeSlider, 
            highCutFreqSlider, highCutSlopeSlider;

        ResponseCurveComponent responseCurveComponent;

        using APVTS = juce::AudioProcessorValueTreeState;
        using Attachment = APVTS::SliderAttachment;

        Attachment peakFreqAttachment, peakGainAttachment, peakQualityAttachment,
            lowCutFreqAttachment, lowCutSlopeAttachment,
            highCutFreqAttachment, highCutSlopeAttachment;

        std::vector<juce::Component*> getComps();

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleEQAudioProcessorEditor)
};
