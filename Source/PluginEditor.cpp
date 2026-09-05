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

    // Теплее -- Ярче
    params.push_back (std::make_unique<AudioParameterFloat>(
        ParameterID { "tone", 1 },
        "Теплее -- Ярче",
        NormalisableRange<float> (-50.0f, 50.0f, 0.1f),
        0.0f));

    // Сжатие вокала
    params.push_back (std::make_unique<AudioParameterFloat>(
        ParameterID { "punch", 1 },
        "Сжатие вокала",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        70.0f));

    // Громкость:
    // 0   = -6 dB
    // 50  =  0 dB
    // 100 = +6 dB
    params.push_back (std::make_unique<AudioParameterFloat>(
        ParameterID { "loudness", 1 },
        "Громкость",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        50.0f));

    // Грязь
    params.push_back (std::make_unique<AudioParameterFloat>(
        ParameterID { "grit", 1 },
        "Грязь",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        40.0f));

    // Пространство
    params.push_back (std::make_unique<AudioParameterFloat>(
        ParameterID { "space", 1 },
        "Пространство",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        45.0f));

    return { params.begin(), params.end() };
}

void VocalChainOneProcessor::prepareToPlay (double sampleRate,
                                            int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec
    {
        sampleRate,
        (juce::uint32) samplesPerBlock,
        1
    };

    for (auto& c : chains)
    {
        // 1. Correction EQ
        c.correctionEq.prepare (spec);

        // 2. Fruity Limiter
        c.limiter.prepare (sampleRate);

        // 3. Parametric EQ 2
        c.eq7.prepare (spec);

        // 4. Fruity Compressor
        c.compressor.prepare (sampleRate);

        // 5. Soundgoodizer C
        c.soundgoodizer.prepare (sampleRate);

        // 6. Fast Dist
        c.fastDist.prepare (sampleRate);

        // 7. Fresh Air
        c.freshAir.prepare (spec);
    }

    juce::dsp::ProcessSpec stereoSpec
    {
        sampleRate,
        (juce::uint32) samplesPerBlock,
        2
    };

    // 8. Flangus
    flangus.prepare (stereoSpec);

    // 9. Reverb
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

    // =========================================================
    // 1-7. MONO / PER-CHANNEL CHAIN
    // =========================================================

    for (auto& c : chains)
    {
        // 1. Initial Correction EQ
        c.correctionEq.update (tone);

        // 2. Fruity Limiter
        //
        // ВАЖНО:
        // громкость сюда НЕ передаём.
        // Лимитер полностью фиксированный.
        c.limiter.update();

        // 3. Fruity Parametric EQ 2
        c.eq7.update (tone);

        // 4. Fruity Compressor
        c.compressor.update (punch);

        // 5. Soundgoodizer
        //
        // Фиксированно:
        // Mode C
        // Amount 45%
        // Wet 100%
        //
        // Никакого пользовательского knob для него нет.
        c.soundgoodizer.update (45.0f);

        // 6. Fast Dist
        c.fastDist.update (grit);

        // 7. Fresh Air
        c.freshAir.update (tone);
    }

    // =========================================================
    // 8-9. STEREO EFFECTS
    // =========================================================

    flangus.update (space);
    reverb.update (space);

    // loudness здесь специально НЕ используется.
}

bool VocalChainOneProcessor::isBusesLayoutSupported (
    const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet()
                == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet()
                == juce::AudioChannelSet::stereo();
}

void VocalChainOneProcessor::processBlock (
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    updateAllStages();

    const int numSamples =
        buffer.getNumSamples();

    auto* left =
        buffer.getWritePointer (0);

    auto* right =
        buffer.getNumChannels() > 1
            ? buffer.getWritePointer (1)
            : nullptr;

    // =========================================================
    // 1-7. ОСНОВНАЯ ЦЕПОЧКА
    // =========================================================

    for (int i = 0; i < numSamples; ++i)
    {
        for (int ch = 0; ch < 2; ++ch)
        {
            float* samplePtr =
                (ch == 0)
                    ? &left[i]
                    : (right != nullptr
                        ? &right[i]
                        : &left[i]);

            float x = *samplePtr;

            // 1. Correction EQ
            x = chains[ch].correctionEq.processSample (0, x);

            // 2. Fruity Limiter
            x = chains[ch].limiter.processSample (x);

            // 3. Parametric EQ 2
            x = chains[ch].eq7.processSample (0, x);

            // 4. Fruity Compressor
            x = chains[ch].compressor.processSample (x);

            // 5. Soundgoodizer C
            x = chains[ch].soundgoodizer.processSample (x);

            // 6. Fast Dist
            x = chains[ch].fastDist.processSample (x);

            // 7. Fresh Air
            x = chains[ch].freshAir.processSample (0, x);

            *samplePtr = x;
        }
    }

    // =========================================================
    // 8. FLANGUS
    // =========================================================

    {
        juce::AudioBuffer<float> dryCopy;
        dryCopy.makeCopyOf (buffer, true);

        juce::dsp::AudioBlock<float> block (buffer);

        flangus.process (block);

        for (int ch = 0;
             ch < buffer.getNumChannels();
             ++ch)
        {
            auto* wet =
                buffer.getWritePointer (ch);

            auto* dry =
                dryCopy.getWritePointer (ch);

            for (int i = 0;
                 i < numSamples;
                 ++i)
            {
                wet[i] =
                    dry[i] * (1.0f - FlangusStage::kWetFixed)
                    + wet[i] * FlangusStage::kWetFixed;
            }
        }
    }

    // =========================================================
    // 9. REVERB
    // =========================================================

    {
        juce::AudioBuffer<float> dryCopy;
        dryCopy.makeCopyOf (buffer, true);

        juce::dsp::AudioBlock<float> block (buffer);

        reverb.process (block);

        for (int ch = 0;
             ch < buffer.getNumChannels();
             ++ch)
        {
            auto* wet =
                buffer.getWritePointer (ch);

            auto* dry =
                dryCopy.getWritePointer (ch);

            for (int i = 0;
                 i < numSamples;
                 ++i)
            {
                wet[i] =
                    dry[i] * (1.0f - ReverbStage::kWetFixed)
                    + wet[i] * ReverbStage::kWetFixed;
            }
        }
    }

    // =========================================================
    // FINAL LOUDNESS
    // =========================================================
    //
    // Это САМОЕ ВАЖНОЕ.
    //
    // Loudness применяется ПОСЛЕ ВСЕЙ ЦЕПОЧКИ.
    //
    // 0   -> -6 dB
    // 50  ->  0 dB
    // 100 -> +6 dB
    //
    // Он НЕ меняет:
    // - Limiter
    // - Compressor
    // - Soundgoodizer
    // - Fast Dist
    // - EQ
    // - Flangus
    // - Reverb
    //
    // Поэтому при увеличении LOUDNESS
    // характер обработки не меняется.
    //

    const float loudness =
        apvts.getRawParameterValue ("loudness")->load();

    const float outputGainDb =
        juce::jmap (loudness,
                    0.0f, 100.0f,
                    -6.0f, 6.0f);

    const float outputGain =
        juce::Decibels::decibelsToGain (
            outputGainDb);

    buffer.applyGain (outputGain);
}

juce::AudioProcessorEditor*
VocalChainOneProcessor::createEditor()
{
    return new VocalChainOneEditor (*this);
}

void VocalChainOneProcessor::getStateInformation (
    juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState();
        state.isValid())
    {
        std::unique_ptr<juce::XmlElement> xml (
            state.createXml());

        copyXmlToBinary (*xml, destData);
    }
}

void VocalChainOneProcessor::setStateInformation (
    const void* data,
    int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (
        getXmlFromBinary (
            data,
            sizeInBytes));

    if (xml != nullptr
        && xml->hasTagName (
            apvts.state.getType()))
    {
        apvts.replaceState (
            juce::ValueTree::fromXml (*xml));
    }
}

juce::AudioProcessor*
JUCE_CALLTYPE createPluginFilter()
{
    return new VocalChainOneProcessor();
}
