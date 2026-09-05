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

    // 0 = -6 dB
    // 50 =  0 dB
    // 100 = +6 dB
    params.push_back (std::make_unique<AudioParameterFloat>(
        ParameterID { "loudness", 1 }, "Громкость",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f));

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

    juce::dsp::ProcessSpec spec {
        sampleRate,
        (juce::uint32) samplesPerBlock,
        1
    };

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

    juce::dsp::ProcessSpec stereoSpec {
        sampleRate,
        (juce::uint32) samplesPerBlock,
        2
    };

    flangus.prepare (stereoSpec);
    reverb.prepare (stereoSpec);

    updateAllStages();
}

void VocalChainOneProcessor::updateAllStages()
{
    const float tone =
        apvts.getRawParameterValue ("tone")->load();

    const float punch =
        apvts.getRawParameterValue ("punch")->load();

    const float loudness =
        apvts.getRawParameterValue ("loudness")->load();

    const float grit =
        apvts.getRawParameterValue ("grit")->load();

    const float space =
        apvts.getRawParameterValue ("space")->load();

    for (auto& c : chains)
    {
        c.correctionEq.update (tone);

        // Limiter НЕ зависит от LOUDNESS.
        // Его текущие настройки остаются фиксированными.
        c.limiter.update (0.0f);

        c.eq7.update (tone);

        c.compressor.update (punch);

        // Soundgoodizer: фиксированный Amount 45%.
        // LOUDNESS на него не влияет.
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

void VocalChainOneProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                           juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    updateAllStages();

    const int numSamples = buffer.getNumSamples();

    auto* left = buffer.getWritePointer (0);
    auto* right =
        buffer.getNumChannels() > 1
        ? buffer.getWritePointer (1)
        : nullptr;

    // =========================================================
    // ШАГИ 1-7
    // Correction EQ
    // Limiter
    // EQ7
    // Compressor
    // Soundgoodizer
    // Fast Dist
    // Fresh Air
    // =========================================================

    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            float* samplePtr =
                (ch == 0)
                ? &left[i]
                : (right != nullptr ? &right[i] : &left[i]);

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

    // =========================================================
    // ШАГ 8: FLANGUS
    // =========================================================

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
            {
                wet[i] =
                    dry[i] * (1.0f - FlangusStage::kWetFixed)
                    + wet[i] * FlangusStage::kWetFixed;
            }
        }
    }

    // =========================================================
    // ШАГ 9: REVERB
    // =========================================================

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
            {
                wet[i] =
                    dry[i] * (1.0f - ReverbStage::kWetFixed)
                    + wet[i] * ReverbStage::kWetFixed;
            }
        }
    }

    // =========================================================
    // ШАГ 10: LOUDNESS / OUTPUT GAIN
    //
    // 0   = -6 dB
    // 50  =  0 dB
    // 100 = +6 dB
    //
    // Это обычное изменение громкости.
    // Оно НЕ меняет работу предыдущих эффектов.
    // =========================================================

    const float loudness =
        apvts.getRawParameterValue ("loudness")->load();

    const float loudnessDb =
        juce::jmap (
            loudness,
            0.0f, 100.0f,
            -6.0f, 6.0f
        );

    const float outputGain =
        juce::Decibels::decibelsToGain (loudnessDb);

    buffer.applyGain (outputGain);
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

void VocalChainOneProcessor::setStateInformation (const void* data,
                                                  int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (
        getXmlFromBinary (data, sizeInBytes)
    );

    if (xml != nullptr
        && xml->hasTagName (apvts.state.getType()))
    {
        apvts.replaceState (
            juce::ValueTree::fromXml (*xml)
        );
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalChainOneProcessor();
}
