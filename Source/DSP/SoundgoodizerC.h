#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>

#include <algorithm>
#include <array>
#include <cmath>

class SoundgoodizerC
{
public:
    void prepare (double newSampleRate, int maximumBlockSize = 512)
    {
        sampleRate = newSampleRate;
        blockSize = maximumBlockSize;

        const int maxDelaySamples =
            juce::jmax (
                8,
                (int) std::ceil (sampleRate * lookaheadSeconds) + 8
            );

        lowDelay.setSize  (1, maxDelaySamples);
        midDelay.setSize  (1, maxDelaySamples);
        highDelay.setSize (1, maxDelaySamples);
        masterDelay.setSize (1, maxDelaySamples);

        lowDelay.clear();
        midDelay.clear();
        highDelay.clear();
        masterDelay.clear();

        lowWrite = 0;
        midWrite = 0;
        highWrite = 0;
        masterWrite = 0;

        lowEnvelope = 0.0f;
        midEnvelope = 0.0f;
        highEnvelope = 0.0f;
        masterEnvelope = 0.0f;

        lowLP.reset();
        midLP.reset();
        highHP.reset();
        masterHP.reset();
    }

    void reset()
    {
        if (lowDelay.getNumSamples() > 0)
            lowDelay.clear();

        if (midDelay.getNumSamples() > 0)
            midDelay.clear();

        if (highDelay.getNumSamples() > 0)
            highDelay.clear();

        if (masterDelay.getNumSamples() > 0)
            masterDelay.clear();

        lowWrite = 0;
        midWrite = 0;
        highWrite = 0;
        masterWrite = 0;

        lowEnvelope = 0.0f;
        midEnvelope = 0.0f;
        highEnvelope = 0.0f;
        masterEnvelope = 0.0f;
    }

    // Existing PluginProcessor interface.
    // Soundgoodizer amount is 0..100.
    void update (float amount)
    {
        amount01 = juce::jlimit (
            0.0f,
            1.0f,
            amount / 100.0f
        );
    }

    // Mono/sample-at-a-time interface.
    float processSample (float x)
    {
        return processMono (x);
    }

    // Existing PluginProcessor interface:
    // processSample (channel, sample)
    float processSample (int /*channel*/, float x)
    {
        return processMono (x);
    }

    // Stereo interface.
    void processSample (float& left, float& right)
    {
        left  = processMono (left);
        right = processMono (right);
    }

    void processStereo (float& left, float& right)
    {
        processSample (left, right);
    }

private:
    static constexpr float lookaheadSeconds = 0.00148f;

    static constexpr float lowFreq  = 200.0f;
    static constexpr float highFreq = 1906.0f;

    // Original Soundgoodizer C amount ≈ 45%.
    static constexpr float defaultAmount = 0.45f;

    double sampleRate = 44100.0;
    int blockSize = 512;

    float amount01 = defaultAmount;

    // -------------------------------------------------------------------------
    // Delay buffers
    // -------------------------------------------------------------------------

    juce::AudioBuffer<float> lowDelay;
    juce::AudioBuffer<float> midDelay;
    juce::AudioBuffer<float> highDelay;
    juce::AudioBuffer<float> masterDelay;

    int lowWrite = 0;
    int midWrite = 0;
    int highWrite = 0;
    int masterWrite = 0;

    // -------------------------------------------------------------------------
    // Envelope state
    // -------------------------------------------------------------------------

    float lowEnvelope = 0.0f;
    float midEnvelope = 0.0f;
    float highEnvelope = 0.0f;
    float masterEnvelope = 0.0f;

    // -------------------------------------------------------------------------
    // Filters
    // -------------------------------------------------------------------------

    juce::dsp::IIR::Filter<float> lowLP;
    juce::dsp::IIR::Filter<float> midLP;
    juce::dsp::IIR::Filter<float> highHP;
    juce::dsp::IIR::Filter<float> masterHP;

    // -------------------------------------------------------------------------
    // Utility
    // -------------------------------------------------------------------------

    static float dbToGain (float db)
    {
        return std::pow (
            10.0f,
            db / 20.0f
        );
    }

    static float softClip (float x, float drive)
    {
        const float driven = x * drive;
        const float normalizer = std::tanh (drive);

        if (normalizer <= 0.000001f)
            return x;

        return std::tanh (driven) / normalizer;
    }

    static float envelopeStep (
        float current,
        float input,
        float attackCoeff,
        float releaseCoeff)
    {
        const float target = std::abs (input);

        if (target > current)
        {
            return current
                + (target - current) * attackCoeff;
        }

        return current
            + (target - current) * releaseCoeff;
    }

    float processBand (
        float x,
        float& envelope,
        float preGainDb,
        float postGainDb,
        float attackMs,
        float releaseMs,
        float sustainMs,
        float ceilingDb,
        bool saturation)
    {
        const float pre = dbToGain (preGainDb);
        const float post = dbToGain (postGainDb);

        float y = x * pre;

        const float attackSeconds =
            juce::jmax (
                0.0001f,
                attackMs * 0.001f
            );

        const float releaseSeconds =
            juce::jmax (
                0.0001f,
                releaseMs * 0.001f
            );

        const float attackCoeff =
            1.0f
            - std::exp (
                -1.0f
                / (float) (sampleRate * attackSeconds)
            );

        const float releaseCoeff =
            1.0f
            - std::exp (
                -1.0f
                / (float) (sampleRate * releaseSeconds)
            );

        envelope =
            envelopeStep (
                envelope,
                y,
                attackCoeff,
                releaseCoeff
            );

        const float sustainSeconds =
            juce::jlimit (
                0.001f,
                1.0f,
                sustainMs * 0.001f
            );

        const float sustainCoeff =
            1.0f
            - std::exp (
                -1.0f
                / (float) (sampleRate * sustainSeconds)
            );

        envelope +=
            (std::abs (y) - envelope)
            * sustainCoeff
            * 0.25f;

        // Moderate compression.
        const float threshold =
            dbToGain (
                -12.0f
                + 7.0f * (1.0f - amount01)
            );

        float gainReduction = 1.0f;

        if (envelope > threshold)
        {
            const float over =
                envelope
                / juce::jmax (
                    threshold,
                    0.000001f
                );

            const float compressed =
                std::pow (
                    juce::jmax (over, 1.0f),
                    0.35f
                );

            gainReduction =
                1.0f
                / juce::jmax (
                    compressed,
                    1.0f
                );
        }

        y *= gainReduction;

        if (saturation)
        {
            const float drive =
                1.15f
                + 0.95f * amount01;

            y = softClip (y, drive);
        }

        y *= post;

        const float ceiling =
            dbToGain (ceilingDb);

        if (std::abs (y) > ceiling)
        {
            y =
                juce::jlimit (
                    -ceiling,
                    ceiling,
                    y
                );
        }

        return y;
    }

    float processMono (float input)
    {
        if (sampleRate <= 0.0)
            return input;

        // ---------------------------------------------------------------------
        // Approximate Maximus-style crossover
        // LOW  < 200 Hz
        // MID  200..1906 Hz
        // HIGH > 1906 Hz
        // ---------------------------------------------------------------------

        const float lowCoeff =
            1.0f
            - std::exp (
                -2.0f
                * juce::MathConstants<float>::pi
                * lowFreq
                / (float) sampleRate
            );

        const float highCoeff =
            1.0f
            - std::exp (
                -2.0f
                * juce::MathConstants<float>::pi
                * highFreq
                / (float) sampleRate
            );

        // Keep separate state per Soundgoodizer instance.
        static thread_local float lowState = 0.0f;
        static thread_local float highState = 0.0f;

        lowState +=
            (input - lowState)
            * lowCoeff;

        highState +=
            (input - highState)
            * highCoeff;

        const float lowBand =
            lowState;

        const float highBand =
            input - highState;

        const float midBand =
            input
            - lowBand
            - highBand;

        // ---------------------------------------------------------------------
        // LOW
        // Original Soundgoodizer C:
        // PRE +8.5
        // POST +1.5
        // ATT 2 ms
        // REL 137 ms
        // SUSTAIN 10 ms
        // CEIL +2.2 dB
        // saturation enabled
        // ---------------------------------------------------------------------

        float lowProcessed =
            processBand (
                lowBand,
                lowEnvelope,
                8.5f,
                1.5f,
                2.0f,
                137.0f,
                10.0f,
                2.2f,
                true
            );

        // ---------------------------------------------------------------------
        // MID
        // PRE +12.7
        // POST +2.8
        // ATT 2 ms
        // REL 85.53 ms
        // SUSTAIN 3.31 ms
        // CEIL 0
        // ---------------------------------------------------------------------

        float midProcessed =
            processBand (
                midBand,
                midEnvelope,
                12.7f,
                2.8f,
                2.0f,
                85.53f,
                3.31f,
                0.0f,
                false
            );

        // ---------------------------------------------------------------------
        // HIGH
        // PRE +11.3
        // POST +2.9
        // ATT 2 ms
        // REL 85.53 ms
        // SUSTAIN 2.18 ms
        // CEIL 0
        // ---------------------------------------------------------------------

        float highProcessed =
            processBand (
                highBand,
                highEnvelope,
                11.3f,
                2.9f,
                2.0f,
                85.53f,
                2.18f,
                0.0f,
                false
            );

        // ---------------------------------------------------------------------
        // Recombine bands.
        // ---------------------------------------------------------------------

        float combined =
            lowProcessed
            + midProcessed
            + highProcessed;

        // ---------------------------------------------------------------------
        // MASTER
        // ATT 2 ms
        // REL 85.53 ms
        // SUSTAIN 10 ms
        // CEIL 0 dB
        // ---------------------------------------------------------------------

        const float masterAttackCoeff =
            1.0f
            - std::exp (
                -1.0f
                / (float) (sampleRate * 0.002)
            );

        const float masterReleaseCoeff =
            1.0f
            - std::exp (
                -1.0f
                / (float) (sampleRate * 0.08553)
            );

        masterEnvelope =
            envelopeStep (
                masterEnvelope,
                combined,
                masterAttackCoeff,
                masterReleaseCoeff
            );

        const float masterCeiling =
            dbToGain (0.0f);

        if (std::abs (combined) > masterCeiling)
        {
            const float reduction =
                masterCeiling
                / juce::jmax (
                    std::abs (combined),
                    0.000001f
                );

            combined *= reduction;
        }

        // ---------------------------------------------------------------------
        // Soundgoodizer big knob ≈ LMH MIX.
        // Original amount ≈ 45%.
        // ---------------------------------------------------------------------

        const float mix =
            juce::jlimit (
                0.0f,
                1.0f,
                amount01
            );

        const float dryAmount =
            1.0f - mix;

        return
            input * dryAmount
            + combined * mix;
    }
};
