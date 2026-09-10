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

        const auto maxDelaySamples =
            juce::jmax (8, (int) std::ceil (sampleRate * lookaheadSeconds) + 8);

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
        if (lowDelay.getNumSamples() > 0)    lowDelay.clear();
        if (midDelay.getNumSamples() > 0)    midDelay.clear();
        if (highDelay.getNumSamples() > 0)   highDelay.clear();
        if (masterDelay.getNumSamples() > 0) masterDelay.clear();

        lowWrite = 0;
        midWrite = 0;
        highWrite = 0;
        masterWrite = 0;

        lowEnvelope = 0.0f;
        midEnvelope = 0.0f;
        highEnvelope = 0.0f;
        masterEnvelope = 0.0f;
    }

    // Compatibility with the existing PluginProcessor.
    void update (float amount)
    {
        amount01 = juce::jlimit (0.0f, 1.0f, amount / 100.0f);
    }

    // Existing processor path: mono/sample-at-a-time.
    float processSample (float x)
    {
        return processMono (x);
    }

    // Compatibility overload for code paths that pass stereo samples.
    // Both samples are processed independently with the same Soundgoodizer state.
    void processSample (float& left, float& right)
    {
        left  = processMono (left);
        right = processMono (right);
    }

    // Convenience stereo version.
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
        return std::pow (10.0f, db / 20.0f);
    }

    static float softClip (float x, float drive)
    {
        const float driven = x * drive;
        const float normalizer = std::tanh (drive);

        if (normalizer <= 0.000001f)
            return x;

        return std::tanh (driven) / normalizer;
    }

    static float envelopeStep (float current,
                               float input,
                               float attackCoeff,
                               float releaseCoeff)
    {
        const float target = std::abs (input);

        if (target > current)
            return current + (target - current) * attackCoeff;

        return current + (target - current) * releaseCoeff;
    }

    float processBand (float x,
                       float& envelope,
                       float preGainDb,
                       float postGainDb,
                       float attackMs,
                       float releaseMs,
                       float sustainMs,
                       float ceilingDb,
                       bool saturation)
    {
        // Original Soundgoodizer C uses strong band PRE gain followed
        // by envelope-based compression/limiting and POST gain.

        const float pre = dbToGain (preGainDb);
        const float post = dbToGain (postGainDb);

        float y = x * pre;

        const float attackSeconds =
            juce::jmax (0.0001f, attackMs * 0.001f);

        const float releaseSeconds =
            juce::jmax (0.0001f, releaseMs * 0.001f);

        const float attackCoeff =
            1.0f - std::exp (-1.0f / (float) (sampleRate * attackSeconds));

        const float releaseCoeff =
            1.0f - std::exp (-1.0f / (float) (sampleRate * releaseSeconds));

        envelope = envelopeStep (
            envelope,
            y,
            attackCoeff,
            releaseCoeff
        );

        // Sustain is represented as a slower RMS-like envelope contribution.
        const float sustainSeconds =
            juce::jlimit (0.001f, 1.0f, sustainMs * 0.001f);

        const float sustainCoeff =
            1.0f - std::exp (-1.0f / (float) (sampleRate * sustainSeconds));

        envelope += (std::abs (y) - envelope) * sustainCoeff * 0.25f;

        // Moderate compression. Amount controls how strongly we approach
        // the original Soundgoodizer-style processed result.
        const float threshold =
            dbToGain (-12.0f + 7.0f * (1.0f - amount01));

        float gainReduction = 1.0f;

        if (envelope > threshold)
        {
            const float over = envelope / juce::jmax (threshold, 0.000001f);

            // Strong but smooth curve, avoiding the huge loudness jump
            // from the previous implementation.
            const float compressed =
                std::pow (juce::jmax (over, 1.0f), 0.35f);

            gainReduction =
                1.0f / juce::jmax (compressed, 1.0f);
        }

        y *= gainReduction;

        if (saturation)
        {
            const float drive =
                1.15f + 0.95f * amount01;

            y = softClip (y, drive);
        }

        y *= post;

        const float ceiling =
            dbToGain (ceilingDb);

        if (std::abs (y) > ceiling)
            y = juce::jlimit (-ceiling, ceiling, y);

        return y;
    }

    float processMono (float input)
    {
        if (sampleRate <= 0.0)
            return input;

        // ---------------------------------------------------------------------
        // Band splitting
        // ---------------------------------------------------------------------

        // Approximate Maximus crossover structure:
        // LOW  < 200 Hz
        // MID  200 .. 1906 Hz
        // HIGH > 1906 Hz
        //
        // These are intentionally gentle so the sum remains stable.
        const float lowCoeff =
            1.0f - std::exp (
                -2.0f * juce::MathConstants<float>::pi
                * lowFreq
                / (float) sampleRate
            );

        const float highCoeff =
            1.0f - std::exp (
                -2.0f * juce::MathConstants<float>::pi
                * highFreq
                / (float) sampleRate
            );

        static float lowState = 0.0f;
        static float highState = 0.0f;

        lowState += (input - lowState) * lowCoeff;

        highState += (input - highState) * highCoeff;

        const float lowBand = lowState;

        const float highBand = input - highState;

        const float midBand =
            input - lowBand - highBand;

        // ---------------------------------------------------------------------
        // Original C-style band settings
        // ---------------------------------------------------------------------

        float lowProcessed =
            processBand (
                lowBand,
                lowEnvelope,
                8.5f,       // PRE
                1.5f,       // POST
                2.0f,       // ATT
                137.0f,     // REL
                10.0f,      // SUSTAIN
                2.2f,       // CEIL
                true        // LOW saturation
            );

        float midProcessed =
            processBand (
                midBand,
                midEnvelope,
                12.7f,      // PRE
                2.8f,       // POST
                2.0f,       // ATT
                85.53f,     // REL
                3.31f,      // SUSTAIN
                0.0f,       // CEIL
                false
            );

        float highProcessed =
            processBand (
                highBand,
                highEnvelope,
                11.3f,      // PRE
                2.9f,       // POST
                2.0f,       // ATT
                85.53f,     // REL
                2.18f,      // SUSTAIN
                0.0f,       // CEIL
                false
            );

        // ---------------------------------------------------------------------
        // MASTER
        // ---------------------------------------------------------------------

        float combined =
            lowProcessed
            + midProcessed
            + highProcessed;

        const float masterAttackCoeff =
            1.0f - std::exp (
                -1.0f /
                (float) (sampleRate * 0.002)
            );

        const float masterReleaseCoeff =
            1.0f - std::exp (
                -1.0f /
                (float) (sampleRate * 0.08553)
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
                masterCeiling /
                juce::jmax (
                    std::abs (combined),
                    0.000001f
                );

            combined *= reduction;
        }

        // ---------------------------------------------------------------------
        // Soundgoodizer big knob = LMH Mix
        // ---------------------------------------------------------------------

        const float mix =
            juce::jlimit (0.0f, 1.0f, amount01);

        // Your original Soundgoodizer amount is approximately 45%.
        // At 45%, this is the processed/dry balance.
        const float dryAmount = 1.0f - mix;

        return input * dryAmount + combined * mix;
    }
};
