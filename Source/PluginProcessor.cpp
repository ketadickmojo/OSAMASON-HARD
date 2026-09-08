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
        ParameterID { "tone", 1 },
        "Теплее -- Ярче",
        NormalisableRange<float> (-50.0f, 50.0f, 0.1f),
        0.0f));

    params.push_back (std::make_unique<AudioParameterFloat>(
        ParameterID { "punch", 1 },
        "Сжатие вокала",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        70.0f));

    params.push_back (std::make_unique<AudioParameterFloat>(
        ParameterID { "loudness", 1 },
        "Громкость",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        50.0f));

    params.push_back (std::make_unique<AudioParameterFloat>(
        ParameterID { "grit", 1 },
        "Грязь",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        40.0f));

    params.push_back (std::make_unique<AudioParameterFloat>(
        ParameterID { "space", 1 },
        "Пространство",
        NormalisableRange<float> (0.0f, 100.0f, 0.1f),
        45.0f));

    return { params.begin(), params.end() };
}

void VocalChainOneProcessor::prepareToPlay (
    double sampleRate,
    int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // ============================================================
    // AUTO-TUNE
    // ============================================================

    autoTune.prepare (
        sampleRate,
        samplesPerBlock);

    // ============================================================
    // MONO PER-CHANNEL DSP
    // ============================================================

    juce::dsp::ProcessSpec spec
    {
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

    // ============================================================
    // STEREO DSP
    // ============================================================

    juce::dsp::ProcessSpec stereoSpec
    {
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

    const float grit =
        apvts.getRawParameterValue ("grit")->load();

    const float space =
        apvts.getRawParameterValue ("space")->load();

    for (auto& c : chains)
    {
        // ========================================================
        // 1. Initial Correction EQ
        // ========================================================

        c.correctionEq.update (tone);

        // ========================================================
        // 2. Fruity Limiter
        // ========================================================

        c.limiter.update();

        // ========================================================
        // 3. Vocal Parametric EQ
        // ========================================================

        c.eq7.update (tone);

        // ========================================================
        // 4. Vintage Compressor
        // ========================================================

        c.compressor.update (punch);

        // ========================================================
        // 5. Soundgoodizer C
        // ========================================================

        c.soundgoodizer.update (45.0f);

        // ========================================================
        // 6. Fast Dist
        // ========================================================

        c.fastDist.update (grit);

        // ========================================================
        // 7. Fresh Air
        // ========================================================

        c.freshAir.update (tone);
    }

    // ============================================================
    // 8. Flangus
    // ============================================================

    flangus.update (space);

    // ============================================================
    // 9. Reverb
    // ============================================================

    reverb.update (space);
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

    const int numSamples =
        buffer.getNumSamples();

    const int numChannels =
        buffer.getNumChannels();

    if (numChannels == 0)
        return;

    // ============================================================
    // UPDATE PARAMETERS
    // ============================================================

    updateAllStages();

    // ============================================================
    // 1. AUTO-TUNE
    //
    // Auto-Tune должен идти ПЕРЕД всей остальной цепочкой.
    //
    // Сейчас движок умеет обнаруживать pitch, но пока ещё
    // не изменяет высоту звука. Поэтому сигнал здесь пока
    // фактически проходит без изменения pitch.
    // ============================================================

    autoTune.process (buffer);

    // ============================================================
    // CHANNEL POINTERS
    // ============================================================

    auto* left =
        buffer.getWritePointer (0);

    auto* right =
        numChannels > 1
            ? buffer.getWritePointer (1)
            : nullptr;

    // ============================================================
    // 2-7. MONO PER-CHANNEL PROCESSING
    //
    // Correction EQ
    // Limiter
    // EQ
    // Compressor
    // Soundgoodizer
    // Fast Dist
    // Fresh Air
    // ============================================================

    for (int i = 0;
         i < numSamples;
         ++i)
    {
        // --------------------------------------------------------
        // LEFT
        // --------------------------------------------------------

        {
            float x =
                left[i];

            x =
                chains[0].correctionEq
                    .processSample (0, x);

            x =
                chains[0].limiter
                    .processSample (x);

            x =
                chains[0].eq7
                    .processSample (0, x);

            x =
                chains[0].compressor
                    .processSample (x);

            x =
                chains[0].soundgoodizer
                    .processSample (x);

            x =
                chains[0].fastDist
                    .processSample (x);

            x =
                chains[0].freshAir
                    .processSample (0, x);

            left[i] = x;
        }

        // --------------------------------------------------------
        // RIGHT
        // --------------------------------------------------------

        if (right != nullptr)
        {
            float x =
                right[i];

            x =
                chains[1].correctionEq
                    .processSample (0, x);

            x =
                chains[1].limiter
                    .processSample (x);

            x =
                chains[1].eq7
                    .processSample (0, x);

            x =
                chains[1].compressor
                    .processSample (x);

            x =
                chains[1].soundgoodizer
                    .processSample (x);

            x =
                chains[1].fastDist
                    .processSample (x);

            x =
                chains[1].freshAir
                    .processSample (0, x);

            right[i] = x;
        }
    }

    // ============================================================
    // 8. FLANGUS
    // ============================================================

    {
        juce::AudioBuffer<float> dryCopy;

        dryCopy.makeCopyOf (
            buffer,
            true);

        juce::dsp::AudioBlock<float> block (
            buffer);

        flangus.process (
            block);

        for (int ch = 0;
             ch < numChannels;
             ++ch)
        {
            auto* wet =
                buffer.getWritePointer (ch);

            const auto* dry =
                dryCopy.getReadPointer (ch);

            for (int i = 0;
                 i < numSamples;
                 ++i)
            {
                wet[i] =
                    dry[i]
                    * (1.0f
                       - FlangusStage::kWetFixed)
                    + wet[i]
                    * FlangusStage::kWetFixed;
            }
        }
    }

    // ============================================================
    // 9. REVERB
    // ============================================================

    {
        juce::AudioBuffer<float> dryCopy;

        dryCopy.makeCopyOf (
            buffer,
            true);

        juce::dsp::AudioBlock<float> block (
            buffer);

        reverb.process (
            block);

        for (int ch = 0;
             ch < numChannels;
             ++ch)
        {
            auto* wet =
                buffer.getWritePointer (ch);

            const auto* dry =
                dryCopy.getReadPointer (ch);

            for (int i = 0;
                 i < numSamples;
                 ++i)
            {
                wet[i] =
                    dry[i]
                    * (1.0f
                       - ReverbStage::kWetFixed)
                    + wet[i]
                    * ReverbStage::kWetFixed;
            }
        }
    }

    // ============================================================
    // FINAL OUTPUT GAIN
    //
    // 0   -> -6 dB
    // 50  ->  0 dB
    // 100 -> +6 dB
    //
    // "Громкость" НЕ меняет limiter/compressor.
    // ============================================================

    const float loudness =
        apvts.getRawParameterValue (
            "loudness")->load();

    const float outputGainDb =
        juce::jmap (
            juce::jlimit (
                0.0f,
                100.0f,
                loudness),
            0.0f,
            100.0f,
            -6.0f,
            6.0f);

    const float outputGain =
        juce::Decibels::decibelsToGain (
            outputGainDb);

    buffer.applyGain (
        outputGain);
}

juce::AudioProcessorEditor*
VocalChainOneProcessor::createEditor()
{
    return new VocalChainOneEditor (*this);
}

void VocalChainOneProcessor::getStateInformation (
    juce::MemoryBlock& destData)
{
    if (auto state =
            apvts.copyState();
        state.isValid())
    {
        std::unique_ptr<juce::XmlElement> xml (
            state.createXml());

        copyXmlToBinary (
            *xml,
            destData);
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
            juce::ValueTree::fromXml (
                *xml));
    }
}

juce::AudioProcessor*
JUCE_CALLTYPE createPluginFilter()
{
    return new VocalChainOneProcessor();
}
