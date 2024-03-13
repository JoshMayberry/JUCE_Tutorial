#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SimpleEQAudioProcessor::SimpleEQAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    )
#endif
{
}

SimpleEQAudioProcessor::~SimpleEQAudioProcessor() {
}

//==============================================================================
const juce::String SimpleEQAudioProcessor::getName() const {
    return JucePlugin_Name;
}

bool SimpleEQAudioProcessor::acceptsMidi() const {
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool SimpleEQAudioProcessor::producesMidi() const {
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool SimpleEQAudioProcessor::isMidiEffect() const {
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double SimpleEQAudioProcessor::getTailLengthSeconds() const {
    return 0.0;
}

int SimpleEQAudioProcessor::getNumPrograms() {
    return 1;
}

int SimpleEQAudioProcessor::getCurrentProgram() {
    return 0;
}

void SimpleEQAudioProcessor::setCurrentProgram(int index) { }

const juce::String SimpleEQAudioProcessor::getProgramName(int index) {
    return {};
}

void SimpleEQAudioProcessor::changeProgramName(int index, const juce::String& newName) {
}

//==============================================================================
void SimpleEQAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) {
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;

    leftChain.prepare(spec);
    rightChain.prepare(spec);

    updateFilters();
}

void SimpleEQAudioProcessor::releaseResources() { }

#ifndef JucePlugin_PreferredChannelConfigurations
bool SimpleEQAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
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
#endif

void SimpleEQAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) {
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear(i, 0, buffer.getNumSamples());
    }

    updateFilters();

    juce::dsp::AudioBlock<float> block(buffer);
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
    leftChain.process(leftContext);
    leftChain.process(rightContext);
}

//==============================================================================
bool SimpleEQAudioProcessor::hasEditor() const {
    return true;
}

juce::AudioProcessorEditor* SimpleEQAudioProcessor::createEditor() {
    return new juce::GenericAudioProcessorEditor(*this);
    //return new SimpleEQAudioProcessorEditor (*this);
}

//==============================================================================
void SimpleEQAudioProcessor::getStateInformation(juce::MemoryBlock& destData) { }

void SimpleEQAudioProcessor::setStateInformation(const void* data, int sizeInBytes) { }

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts) {
    ChainSettings settings;

    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq")->load();
    settings.highCutFreq = apvts.getRawParameterValue("HighCut Freq")->load();
    settings.peakFreq = apvts.getRawParameterValue("Peak Freq")->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue("Peak Gain")->load();
    settings.peakQuality = apvts.getRawParameterValue("Peak Quality")->load();
    settings.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue("LowCut Slope")->load());
    settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue("HighCut Slope")->load());

    return settings;
}

void SimpleEQAudioProcessor::updatePeakFilter(const ChainSettings& chainSettings) {
    auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
        getSampleRate(),
        chainSettings.peakFreq,
        chainSettings.peakQuality,
        juce::Decibels::decibelsToGain(chainSettings.peakGainInDecibels)
    );

    leftChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
    rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
}

void SimpleEQAudioProcessor::updateLowCutFilters(const ChainSettings& chainSettings) {
    auto lowCutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(
        chainSettings.lowCutFreq,
        getSampleRate(),
        2 * (chainSettings.lowCutSlope + 1)
    );

    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();

    updateCutFilter(leftLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
    updateCutFilter(leftLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
};

void SimpleEQAudioProcessor::updateHighCutFilters(const ChainSettings& chainSettings) {
    auto highCutCoefficients = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(
        chainSettings.highCutFreq,
        getSampleRate(),
        2 * (chainSettings.highCutSlope + 1)
    );

    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();

    updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);
    updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);
};

void SimpleEQAudioProcessor::updateFilters() {
    auto chainSettings = getChainSettings(apvts);

    updatePeakFilter(chainSettings);
    updateLowCutFilters(chainSettings);
    updateHighCutFilters(chainSettings);
};

juce::AudioProcessorValueTreeState::ParameterLayout SimpleEQAudioProcessor::createParameterLayout() {
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "LowCut Freq",
        "LowCut Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f),
        20.f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "HighCut Freq",
        "HighCut Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f),
        20000.f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "Peak Freq",
        "Peak Freq",
        juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f),
        750.f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "Peak Gain",
        "Peak Gain",
        juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
        0.0f
    ));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "Peak Quality",
        "Peak Quality",
        juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
        1.0f
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "LowCut Slope",
        "LowCut Slope",
        juce::StringArray({
            "12 db/Oct",
            "24 db/Oct",
            "36 db/Oct",
            "48 db/Oct"
            }),
        0
    ));

    layout.add(std::make_unique<juce::AudioParameterChoice>(
        "HighCut Slope",
        "HighCut Slope",
        juce::StringArray({
            "12 db/Oct",
            "24 db/Oct",
            "36 db/Oct",
            "48 db/Oct"
            }),
        0
    ));

    return layout;
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new SimpleEQAudioProcessor();
}
