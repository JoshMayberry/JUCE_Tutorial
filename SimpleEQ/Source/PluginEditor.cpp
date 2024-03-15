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

void LookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& toggleButton, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) {
    using namespace juce;

    auto bounds = toggleButton.getLocalBounds();

    if (auto* pb = dynamic_cast<PowerButton*>(&toggleButton)) {
        auto size = jmin(bounds.getWidth(), bounds.getHeight()) - 6;
        auto r = bounds.withSizeKeepingCentre(size, size).toFloat();

        float ang = 30.f;
        size -= 6;

        // FOR DEBUGGING
        // g.setColour(Colours::red);
        // g.drawRect(bounds);
        // END DEBUGGING

        Path powerButton;
        powerButton.addCentredArc(r.getCentreX(), r.getCentreY(), size * 0.5, size * 0.5, 0.f, degreesToRadians(ang), degreesToRadians(360 - ang), true);
        powerButton.startNewSubPath(r.getCentreX(), r.getY());
        powerButton.lineTo(r.getCentre());
        PathStrokeType pst(2.f, PathStrokeType::JointStyle::curved);

        auto color = toggleButton.getToggleState() ? Colours::dimgrey : Colour(0u, 172u, 1u);
        g.setColour(color);
        g.strokePath(powerButton, pst);
        g.drawEllipse(r, 2);
    }
    else if (auto* analyzerButton = dynamic_cast<AnalyzerButton*>(&toggleButton)) {
        auto color = toggleButton.getToggleState() ? Colour(0u, 172u, 1u) : Colours::dimgrey;
        g.setColour(color);
        g.drawRect(bounds);
        
        auto insetRect = bounds.reduced(4);
        Path randomPath;
        Random r;
        randomPath.startNewSubPath(insetRect.getX(), insetRect.getY() + insetRect.getHeight() * r.nextFloat());
        for (auto x = insetRect.getX() + 1; x < insetRect.getRight(); x += 2) {
            randomPath.lineTo(x, insetRect.getY() + insetRect.getHeight() * r.nextFloat());
        }
        g.strokePath(randomPath, PathStrokeType(1.f));
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
ResponseCurveComponent::ResponseCurveComponent(SimpleEQAudioProcessor& p) : AudioProcessorEditor(&p),
    audioProcessor(p),
    leftPathProducer(audioProcessor.leftChannelFifo),
    rightPathProducer(audioProcessor.rightChannelFifo)
{
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

void PathProducer::process(juce::Rectangle<float> fftBounds, double sampleRate) {
    juce::AudioBuffer<float> tempIncomingBuffer;

    while (leftChannelFifo->getNumCompleteBuffersAvailable() > 0) {
        if (leftChannelFifo->getAudioBuffer(tempIncomingBuffer)) {
            auto size = tempIncomingBuffer.getNumSamples();
            juce::FloatVectorOperations::copy(
                monoBuffer.getWritePointer(0, 0),
                monoBuffer.getReadPointer(0, size),
                monoBuffer.getNumSamples() - size
            );

            juce::FloatVectorOperations::copy(
                monoBuffer.getWritePointer(0, monoBuffer.getNumSamples() - size),
                tempIncomingBuffer.getReadPointer(0, 0),
                size
            );

            leftChannelFFTDataGenerator.produceFFTDataForRendering(monoBuffer, -48.f);
        }
    }

    // If there are FFD data buffers to pull and we can pull a buffer, generate a path
    const auto fftSize = leftChannelFFTDataGenerator.getFFTSize();
    const auto binWidth = sampleRate / (double)fftSize;
    while (leftChannelFFTDataGenerator.getNumAvailableFFTDataBlocks() > 0) {
        std::vector<float> fftData;
        if (leftChannelFFTDataGenerator.getFFTData(fftData)) {
            pathProducer.generatePath(fftData, fftBounds, fftSize, binWidth, -48.f);
        }
    }

    // While there are paths that can be pulled, pull as many as we can and display the most recent path
    while (pathProducer.getNumPathsAvailable()) {
        pathProducer.getPath(leftChannelFFTPath);
    }
}

void ResponseCurveComponent::timerCallback() {
    auto fftBounds = getAnalysisArea().toFloat();
    auto sampleRate = audioProcessor.getSampleRate();
    leftPathProducer.process(fftBounds, sampleRate);
    rightPathProducer.process(fftBounds, sampleRate);

    if (parametersChanged.compareAndSetBool(false, true)) {
        updateChain();
    }

    repaint();
}

void ResponseCurveComponent::updateChain() {
    auto chainSettings = getChainSettings(audioProcessor.apvts);

    monoChain.setBypassed<ChainPositions::LowCut>(chainSettings.lowCutBypassed);
    monoChain.setBypassed<ChainPositions::HighCut>(chainSettings.highCutBypassed);
    monoChain.setBypassed<ChainPositions::Peak>(chainSettings.peakBypassed);

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

    g.drawImage(background, getLocalBounds().toFloat());

    auto responseArea = getAnalysisArea();

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

        if (!monoChain.isBypassed <ChainPositions::LowCut>()) {
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
        }

        if (!monoChain.isBypassed <ChainPositions::HighCut>()) {
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

    auto leftChannelFFTPath = leftPathProducer.getPath();
    leftChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));
    g.setColour(Colours::skyblue);
    g.strokePath(leftChannelFFTPath, PathStrokeType(1.f));

    auto rightChannelFFTPath = rightPathProducer.getPath();
    rightChannelFFTPath.applyTransform(AffineTransform().translation(responseArea.getX(), responseArea.getY()));
    g.setColour(Colours::lightyellow);
    g.strokePath(rightChannelFFTPath, PathStrokeType(1.f));

    g.setColour(Colours::orange);
    g.drawRoundedRectangle(getRenderArea().toFloat(), 4.f, 1.f);

    g.setColour(Colours::white);
    g.strokePath(responseCurve, PathStrokeType(2.f));
}

void ResponseCurveComponent::resized() {
    using namespace juce;
    background = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);

    Graphics g(background);

    // FOR DEBUGGING
    // g.drawRect(getRenderArea());
    // g.drawRect(renderArea);
    // END DEBUGGING

    // Cache render area sizes
    auto renderArea = getAnalysisArea();
    auto left = renderArea.getX();
    auto right = renderArea.getRight();
    auto top = renderArea.getY();
    auto bottom = renderArea.getBottom();
    auto width = renderArea.getWidth();

    // Draw Lines
    Array<float> freqs {
        20, 50, 100,
        200, 500, 1000,
        2000, 5000, 10000,
        20000
    };

    Array<float> xs;
    for (auto f : freqs) {
        auto normX = mapFromLog10(f, 20.f, 20000.f);
        xs.add(left + width * normX);
    }

    g.setColour(Colours::dimgrey);
    for (auto x : xs) {
        g.drawVerticalLine(x, top, bottom);
    }

    Array<float> gain {
        -24, -12, 0, 12, 24
    };

    for (auto gDb : gain) {
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::darkgrey);
        g.drawHorizontalLine(y, left, right);
    }

    // Draw Labels
    g.setColour(Colours::lightgrey);
    const int fontHeight = 10;
    g.setFont(fontHeight);
    for (int i = 0; i < freqs.size(); ++i) {
        auto f = freqs[i];
        auto x = xs[i];

        bool addK = false;
        String str;
        if (f > 999.f) {
            addK = true;
            f /= 1000.f;
        }

        str << f;
        if (addK) {
            str << "k";
        }
        str << "Hz";

        auto textWidth = g.getCurrentFont().getStringWidth(str);
        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setCentre(x, 0);
        r.setY(1);

        g.drawFittedText(str, r, juce::Justification::verticallyCentred, 1);
    }

    for (auto gDb : gain) {
        auto y = jmap(gDb, -24.f, 24.f, float(bottom), float(top));
        String str;
        if (gDb > 0) {
            str << "+";
        }
        str << gDb;

        auto textWidth = g.getCurrentFont().getStringWidth(str);
        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setX(getWidth() - textWidth);
        r.setCentre(r.getCentreX(), y);

        g.setColour(gDb == 0.f ? Colour(0u, 172u, 1u) : Colours::lightgrey);
        g.drawFittedText(str, r, juce::Justification::verticallyCentred, 1);

        // Do labels for analyzer
        str.clear();
        str << (gDb - 24.f);

        r.setX(1);
        textWidth = g.getCurrentFont().getStringWidth(str);
        r.setSize(textWidth, fontHeight);
        g.setColour(Colours::lightgrey);
        g.drawFittedText(str, r, juce::Justification::verticallyCentred, 1);
    }
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea() {
    auto bounds = getLocalBounds();

    bounds.removeFromTop(12);
    bounds.removeFromBottom(2);
    bounds.removeFromLeft(20);
    bounds.removeFromRight(20);

    return bounds;
}

juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea() {
    auto bounds = getRenderArea();

    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);

    return bounds;
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
    highCutSlopeAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider),
    lowCutBypassButtonAttachment(audioProcessor.apvts, "LowCut Bypassed", lowCutBypassButton),
    highCutBypassButtonAttachment(audioProcessor.apvts, "HighCut Bypassed", highCutBypassButton),
    peakBypassButtonAttachment(audioProcessor.apvts, "Peak Bypassed", peakBypassButton),
    analyzerEnabledButtonAttachment(audioProcessor.apvts, "Analyzer Enabled", analyzerEnabledButton)
{

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

    lowCutBypassButton.setLookAndFeel(&lnf);
    highCutBypassButton.setLookAndFeel(&lnf);
    peakBypassButton.setLookAndFeel(&lnf);
    analyzerEnabledButton.setLookAndFeel(&lnf);

    setSize (600, 480);
}

SimpleEQAudioProcessorEditor::~SimpleEQAudioProcessorEditor() {
    lowCutBypassButton.setLookAndFeel(nullptr);
    highCutBypassButton.setLookAndFeel(nullptr);
    peakBypassButton.setLookAndFeel(nullptr);
    analyzerEnabledButton.setLookAndFeel(nullptr);
}

//==============================================================================
void SimpleEQAudioProcessorEditor::paint (juce::Graphics& g) {
    using namespace juce;

    g.fillAll (Colours::black);
}

void SimpleEQAudioProcessorEditor::resized() {
    auto bounds = getLocalBounds();

    auto analyzerEnabledArea = bounds.removeFromTop(25);
    analyzerEnabledArea.setWidth(100);
    analyzerEnabledArea.setX(5);
    analyzerEnabledArea.removeFromTop(2);
    analyzerEnabledButton.setBounds(analyzerEnabledArea);
    bounds.removeFromTop(5);

    float hRatio = 25.f / 100.f; // JUCE_LIVE_CONSTANT(25) / 100.f;
    responseCurveComponent.setBounds(bounds.removeFromTop(bounds.getHeight() * hRatio));

    bounds.removeFromTop(5); // Space out components

    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    lowCutBypassButton.setBounds(lowCutArea.removeFromTop(25));
    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);

    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);
    highCutBypassButton.setBounds(highCutArea.removeFromTop(25));
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    highCutSlopeSlider.setBounds(highCutArea);

    peakBypassButton.setBounds(bounds.removeFromTop(25));
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
        &responseCurveComponent,
        &lowCutBypassButton,
        &highCutBypassButton,
        &peakBypassButton,
        &analyzerEnabledButton
    };
}

