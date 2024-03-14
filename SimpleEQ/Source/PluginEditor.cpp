/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

void LookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) {
    using namespace juce;

    auto bounds = Rectangle<float>(x, y, width, height);

    g.setColour(Colour(97u, 18u, 167u));
    g.fillEllipse(bounds);

    g.setColour(Colour(255u, 154u, 1u));
    g.drawEllipse(bounds, 1.f);

    if (auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider)) {
        // Add Line
        auto center = bounds.getCentre();
        Path p;

        Rectangle<float> r;
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom(center.getY() - rswl->getTextHeight() * 1.5);
        p.addRoundedRectangle(r, 2.f);

        jassert(rotaryStartAngle < rotaryEndAngle);
        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);
        p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));

        g.fillPath(p);

        // Add Text
        g.setFont(rswl->getTextHeight());
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);

        r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
        r.setCentre(bounds.getCentre());

        g.setColour(Colours::black);
        g.fillRect(r);

        g.setColour(Colours::white);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

//==============================================================================
void RotarySliderWithLabels::paint(juce::Graphics &g) {
    using namespace juce;

    auto startAng = degreesToRadians(180.f + 45.f);
    auto endAng = degreesToRadians(180.f - 45.f) + MathConstants<float>::twoPi;

    auto range = getRange();
    auto sliderBounds = getSliderBounds();

    // FOR DEBUGGING
    // g.setColour(Colours::red);
    // g.drawRect(getLocalBounds());
    // g.setColour(Colours::yellow);
    // g.drawRect(sliderBounds);
    // END DEBUGGING

    getLookAndFeel().drawRotarySlider(
        g,
        sliderBounds.getX(),
        sliderBounds.getY(),
        sliderBounds.getWidth(),
        sliderBounds.getHeight(),
        jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
        startAng,
        endAng,
        *this
    );

    // Min Max Labels
    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * 0.5f;

    g.setColour(Colour(0u, 172u, 1u));
    g.setFont(getTextHeight());

    auto numChoices = labels.size();
    for (int i = 0; i < numChoices; ++i) {
        auto pos = labels[i].pos;
        jassert(0.f <= pos);
        jassert(pos <= 1.f);

        auto ang = jmap(pos, 0.f, 1.f, startAng, endAng);
        auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, ang);
        Rectangle<float> r;
        auto str = labels[i].label;
        r.setSize(g.getCurrentFont().getStringWidth(str), getTextHeight());
        r.setCentre(c);
        r.setY(r.getY() + getTextHeight());

        g.drawText(str, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const {
    auto bounds = getLocalBounds();
    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
    size -= getTextHeight() * 2;
    
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(2);

    return r;
}

juce::String RotarySliderWithLabels::getDisplayString() const {
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param)) {
        return choiceParam->getCurrentChoiceName();
    }

    juce::String str;
    bool addK = false;

    if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param)) {
        float val = getValue();

        if (val > 999.f) {
            val /= 1000.f;
            addK = true;
        }

        str = juce::String(val, (addK ? 2 : 0));
    }
    else {
        jassertfalse; // This should not happen
    }

    if (suffix.isNotEmpty()) {
        str << " ";
        if (addK) {
            str << "k";
        }
    }

    return str;
}

//==============================================================================
ResponseCurveComponent::ResponseCurveComponent(SimpleEQAudioProcessor& p) : AudioProcessorEditor(&p), audioProcessor(p) {
    const auto& params = audioProcessor.getParameters();
    for (auto param : params) {
        param->addListener(this);
    }

    updateChain();
    startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent() {
    const auto& params = audioProcessor.getParameters();
    for (auto param : params) {
        param->removeListener(this);
    }
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue) {
    parametersChanged.set(true); // This function gets called a lot- so no logic goes here
}

void ResponseCurveComponent::timerCallback() {
    if (parametersChanged.compareAndSetBool(false, true)) {
        updateChain();
    }
}

void ResponseCurveComponent::updateChain() {
    auto chainSettings = getChainSettings(audioProcessor.apvts);
    auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
    monoChain.get<ChainPositions::Peak>().coefficients = peakCoefficients;

    auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
    auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
    updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);

    repaint();
}

void ResponseCurveComponent::paint(juce::Graphics& g) {
    using namespace juce;

    g.fillAll(Colours::black);

    auto responseArea = getLocalBounds();

    auto sampleRate = audioProcessor.getSampleRate();

    auto& lowCut = monoChain.get<ChainPositions::LowCut>();
    auto& peak = monoChain.get<ChainPositions::Peak>();
    auto& highCut = monoChain.get<ChainPositions::HighCut>();

    auto w = responseArea.getWidth();
    std::vector<double> mags;
    mags.resize(w);

    for (int i = 0; i < w; i++) {
        double mag = 1.f;
        auto freq = mapToLog10(double(i) / double(w), 20.0, 20000.0);
        if (!monoChain.isBypassed<ChainPositions::Peak>()) {
            mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        if (!lowCut.isBypassed<0>()) {
            mag *= lowCut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!lowCut.isBypassed<1>()) {
            mag *= lowCut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!lowCut.isBypassed<2>()) {
            mag *= lowCut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!lowCut.isBypassed<3>()) {
            mag *= lowCut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        if (!highCut.isBypassed<0>()) {
            mag *= highCut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!highCut.isBypassed<1>()) {
            mag *= highCut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!highCut.isBypassed<2>()) {
            mag *= highCut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }
        if (!highCut.isBypassed<3>()) {
            mag *= highCut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        }

        mags[i] = Decibels::gainToDecibels(mag);
    }

    Path responseCurve;

    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input) {
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };

    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));
    for (size_t i = 1; i < mags.size(); ++i) {
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
    }
    g.setColour(Colours::orange);
    g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f);

    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));
}

//==============================================================================
SimpleEQAudioProcessorEditor::SimpleEQAudioProcessorEditor (SimpleEQAudioProcessor& p) : AudioProcessorEditor (&p),
    peakFreqSlider(*audioProcessor.apvts.getParameter("Peak Freq"), "Hz"),
    peakGainSlider(*audioProcessor.apvts.getParameter("Peak Gain"), "dB"),
    peakQualitySlider(*audioProcessor.apvts.getParameter("Peak Quality"), ""),
    lowCutFreqSlider(*audioProcessor.apvts.getParameter("LowCut Freq"), "Hz"),
    lowCutSlopeSlider(*audioProcessor.apvts.getParameter("LowCut Slope"), "Hz"),
    highCutFreqSlider(*audioProcessor.apvts.getParameter("HighCut Freq"), "dB/Oct"),
    highCutSlopeSlider(*audioProcessor.apvts.getParameter("HighCut Slope"), "db/Oct"),
    audioProcessor(p),
    responseCurveComponent(audioProcessor),
    peakFreqAttachment(audioProcessor.apvts, "Peak Freq", peakFreqSlider),
    peakGainAttachment(audioProcessor.apvts, "Peak Gain", peakGainSlider),
    peakQualityAttachment(audioProcessor.apvts, "Peak Quality", peakQualitySlider),
    lowCutFreqAttachment(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
    lowCutSlopeAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
    highCutFreqAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
    highCutSlopeAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider) {

    peakFreqSlider.labels.add({ 0.f, "20 Hz" });
    peakFreqSlider.labels.add({ 1.f, "20 kHz" });
    peakGainSlider.labels.add({ 0.f, "-24 dB" });
    peakGainSlider.labels.add({ 1.f, "24 dB" });
    peakQualitySlider.labels.add({ 0.f, "0.1" });
    peakQualitySlider.labels.add({ 1.f, "10.0" });

    lowCutFreqSlider.labels.add({ 0.f, "20 Hz" });
    lowCutFreqSlider.labels.add({ 1.f, "20 kHz" });
    lowCutSlopeSlider.labels.add({ 0.f, "12" });
    lowCutSlopeSlider.labels.add({ 1.f, "48" });

    highCutFreqSlider.labels.add({ 0.f, "20 Hz" });
    highCutFreqSlider.labels.add({ 1.f, "20 kHz" });
    highCutSlopeSlider.labels.add({ 0.f, "12" });
    highCutSlopeSlider.labels.add({ 1.f, "48" });

    for (auto* comp : getComps()) {
        addAndMakeVisible(comp);
    }

    setSize (600, 480);
}

SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor() { }

//==============================================================================
void SimpleEQAudioProcessorEditor::paint (juce::Graphics& g) {
    using namespace juce;

    g.fillAll (Colours::black);
}

void SimpleEQAudioProcessorEditor::resized() {
    auto bounds = getLocalBounds();
    float hRatio = 25.f / 100.f; // JUCE_LIVE_CONSTANT(25) / 100.f;
    responseCurveComponent.setBounds(bounds.removeFromTop(bounds.getHeight() * hRatio));

    bounds.removeFromTop(5); // Space out components

    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);

    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    highCutSlopeSlider.setBounds(highCutArea);

    peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.33));
    peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight() * 0.5));
    peakQualitySlider.setBounds(bounds);
}

std::vector<juce::Component*> SimpleEQAudioProcessorEditor::getComps() {
    return {
        &peakFreqSlider,
        &peakGainSlider,
        &peakQualitySlider,
        &lowCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutFreqSlider,
        &highCutSlopeSlider,
        &responseCurveComponent
    };
}

