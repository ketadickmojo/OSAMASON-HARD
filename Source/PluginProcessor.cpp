#include "PluginProcessor.h"
#include "PluginEditor.h"

VocalChainOneProcessor::VocalChainOneProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout VocalChainOneProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    params.push_back (std::make_unique<AudioParameterFloat>(
        ParameterID { "tone", 1 }, "Теплее -- Ярче",
        NormalisableRange<float> (-50.0f, 50.0f, 0.1f), 0.0f));

    params.push_back (std::make_unique<AudioParameterFloat>(
        ParameterID { "punch", 1 }, "Сжатие вокала",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f), 70.0f));

    params.push_back (std::make_unique<AudioParameterFloat>(
        ParameterID { "loudness", 1 }, "Громкость",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f), 80.0f));

    params.push_back (std::make_unique<AudioParameterFloat>(
        ParameterID { "grit", 1 }, "Грязь",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f), 40.0f));

    params.push_back (std::make_unique<AudioParameterFloat>(
        ParameterID { "space", 1 }, "Пространство",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f), 45.0f));

    return { params.begin(), params.end() };
}

void VocalChainOneProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) samplesPerBlock, 1 }; // per-channel spec (numChannels=1 для per-channel цепей)

    for (auto& c : chains)
    {
        c.correctionEq.prepare (spec);
        c.limiter.prepare (sampleRate);
        c.eq7.prepare (spec);
        c.compressor.prepare (sampleRate);
        c.soundgoodizer.prepare (sampleRate);
        c.fastDist.prepare (sampleRate);
        c.freshAir.prepare (spec);
    }

    juce::dsp::ProcessSpec stereoSpec { sampleRate, (juce::uint32) samplesPerBlock, 2 };
    flangus.prepare (stereoSpec);
    reverb.prepare (stereoSpec);

    updateAllStages();
}

void VocalChainOneProcessor::updateAllStages()
{
    float tone     = apvts.getRawParameterValue ("tone")->load();
    float punch    = apvts.getRawParameterValue ("punch")->load();
    float loudness = apvts.getRawParameterValue ("loudness")->load();
    float grit     = apvts.getRawParameterValue ("grit")->load();
    float space    = apvts.getRawParameterValue ("space")->load();

    for (auto& c : chains)
{
    c.correctionEq.update (tone);
    c.limiter.update (loudness);
    c.eq7.update (tone);
    c.compressor.update (punch);
    c.soundgoodizer.update (45.0f);
    c.fastDist.update (grit);
    c.freshAir.update (tone);
}

    flangus.update (space);
    reverb.update (space);
}

bool VocalChainOneProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet()  == juce::AudioChannelSet::stereo();
}

void VocalChainOneProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    updateAllStages();

    const int numSamples = buffer.getNumSamples();
    auto* left  = buffer.getWritePointer (0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;

    // Шаги 1-7 (CorrectionEQ .. FreshAir) -- поканально, простая обработка сэмпл-за-сэмплом
    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            float* samplePtr = (ch == 0) ? &left[i] : (right != nullptr ? &right[i] : &left[i]);
            float x = *samplePtr;

            x = chains[ch].correctionEq.processSample (0, x);
            x = chains[ch].limiter.processSample (x);
            x = chains[ch].eq7.processSample (0, x);
            x = chains[ch].compressor.processSample (x);
            x = chains[ch].soundgoodizer.processSample (x);
            x = chains[ch].fastDist.processSample (x);
            x = chains[ch].freshAir.processSample (0, x);

            *samplePtr = x;
        }
    }

    // Шаг 8: Flangus (стерео-блок, wet зафиксирован на 17%)
    {
        juce::AudioBuffer<float> dryCopy;
        dryCopy.makeCopyOf (buffer, true);

        juce::dsp::AudioBlock<float> block (buffer);
        flangus.process (block);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* wet = buffer.getWritePointer (ch);
            auto* dry = dryCopy.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
                wet[i] = dry[i] * (1.0f - FlangusStage::kWetFixed) + wet[i] * FlangusStage::kWetFixed;
        }
    }

    // Шаг 9: Fruity Reverb 2 (стерео-блок, wet зафиксирован на 6%)
    {
        juce::AudioBuffer<float> dryCopy;
        dryCopy.makeCopyOf (buffer, true);

        juce::dsp::AudioBlock<float> block (buffer);
        reverb.process (block);

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* wet = buffer.getWritePointer (ch);
            auto* dry = dryCopy.getWritePointer (ch);
            for (int i = 0; i < numSamples; ++i)
                wet[i] = dry[i] * (1.0f - ReverbStage::kWetFixed) + wet[i] * ReverbStage::kWetFixed;
        }
    }
}

juce::AudioProcessorEditor* VocalChainOneProcessor::createEditor()
{
    return new VocalChainOneEditor (*this);
}

void VocalChainOneProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        std::unique_ptr<juce::XmlElement> xml (state.createXml());
        copyXmlToBinary (*xml, destData);
    }
}

void VocalChainOneProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalChainOneProcessor();
}
