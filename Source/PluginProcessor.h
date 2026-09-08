#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "DSP/CorrectionEQ.h"
#include "DSP/SimpleLimiter.h"
#include "DSP/VocalParametricEQ7.h"
#include "DSP/VintageCompressor.h"
#include "DSP/SoundgoodizerC.h"
#include "DSP/FastDistStage.h"
#include "DSP/FreshAirStage.h"
#include "DSP/FlangusStage.h"
#include "DSP/ReverbStage.h"

#include "AutoTuneEngine.h"

class VocalChainOneProcessor : public juce::AudioProcessor
{
public:
    VocalChainOneProcessor();
    ~VocalChainOneProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override
    {
        return "VocalChainOne";
    }

    bool acceptsMidi() const override
    {
        return false;
    }

    bool producesMidi() const override
    {
        return false;
    }

    double getTailLengthSeconds() const override
    {
        return 1.5;
    }

    int getNumPrograms() override
    {
        return 1;
    }

    int getCurrentProgram() override
    {
        return 0;
    }

    void setCurrentProgram (int) override {}

    const juce::String getProgramName (int) override
    {
        return {};
    }

    void changeProgramName (
        int,
        const juce::String&) override {}

    void getStateInformation (
        juce::MemoryBlock& destData) override;

    void setStateInformation (
        const void* data,
        int sizeInBytes) override;

    bool isBusesLayoutSupported (
        const BusesLayout& layouts) const override;

    juce::AudioProcessorValueTreeState apvts;

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    void updateAllStages();

    // ============================================================
    // ОСНОВНАЯ ЦЕПОЧКА
    //
    // Auto-Tune
    //      ↓
    // CorrectionEQ
    //      ↓
    // Limiter
    //      ↓
    // EQ7
    //      ↓
    // Compressor
    //      ↓
    // Soundgoodizer
    //      ↓
    // Fast Dist
    //      ↓
    // Fresh Air
    //      ↓
    // Flangus
    //      ↓
    // Reverb
    //      ↓
    // Output
    //
    // Порядок фиксированный.
    // ============================================================

    struct ChannelChain
    {
        CorrectionEQ correctionEq;
        SimpleLimiter limiter;
        VocalParametricEQ7 eq7;
        VintageCompressor compressor;
        SoundgoodizerC soundgoodizer;
        FastDistStage fastDist;
        FreshAirStage freshAir;
    };

    // ============================================================
    // AUTO-TUNE
    // ============================================================

    AutoTuneEngine autoTune;

    // ============================================================
    // VOCAL CHAIN
    // ============================================================

    ChannelChain chains[2];

    // Стерео-обработка после пер-канальной части.
    FlangusStage flangus;
    ReverbStage reverb;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (
        VocalChainOneProcessor)
};
