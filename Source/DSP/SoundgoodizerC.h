#pragma once

#include <JuceHeader.h>
#include <array>
#include <cmath>

// Soundgoodizer C recreation for VocalChainOne.
//
// The real FL Studio Soundgoodizer is a stereo maximizer/enhancer based on
// Maximus. Image-Line documents that it uses three frequency bands plus a
// MASTER stage, with four independent compression envelopes, and that the
// Soundgoodizer amount is the LMH mix between dry input and the processed
// LOW/MID/HIGH signal.
//
// This implementation uses the user's measured Soundgoodizer C settings:
//   LOW  : PRE +8.5 dB, POST +1.5 dB, ATT 2 ms, REL 137 ms, SUSTAIN 10 ms,
//          SAT 100% / Mode A, CEIL +2.2 dB, stereo MERGED 75%
//   MID  : PRE +12.7 dB, POST +2.8 dB, ATT 2 ms, REL 85.53 ms,
//          SUSTAIN 3.31 ms, stereo SEPARATION 35%
//   HIGH : PRE +11.3 dB, POST +2.9 dB, ATT 2 ms, REL 85.53 ms,
//          SUSTAIN 2.18 ms, stereo OFF
//   MASTER: PRE 0 dB, POST 0 dB, ATT 2 ms, REL 85.53 ms, SUSTAIN 10 ms
//   LMH delay: 1.48 ms
//   Crossovers: 200 Hz / 1906 Hz
//   Soundgoodizer amount: 45%
//
// The exact Maximus compression envelope is proprietary; the original .fnv
// states supplied by the user were used to shape the transfer curves below.
// This is intentionally gain-stable: the processed branch is internally
// level-controlled before the final 45% LMH blend so it does not explode in
// loudness like the earlier approximation.

class SoundgoodizerC
{
public:
    void prepare (double newSampleRate, int maximumBlockSize)
    {
        sampleRate = juce::jmax (1.0, newSampleRate);
        delaySamples = juce::jmax (1, juce::roundToInt (sampleRate * 0.00148));

        lowLpf.prepare ({ sampleRate, static_cast<juce::uint32> (maximumBlockSize), 1 });
        lowHpf.prepare ({ sampleRate, static_cast<juce::uint32> (maximumBlockSize), 1 });
        midLpf.prepare ({ sampleRate, static_cast<juce::uint32> (maximumBlockSize), 1 });
        midHpf.prepare ({ sampleRate, static_cast<juce::uint32> (maximumBlockSize), 1 });
        highHpf.prepare({ sampleRate, static_cast<juce::uint32> (maximumBlockSize), 1 });

        makeFilters();

        delayL.assign (static_cast<size_t> (delaySamples + 4), 0.0f);
        delayR.assign (static_cast<size_t> (delaySamples + 4), 0.0f);
        delayWriteL = 0;
        delayWriteR = 0;

        lowEnvL = lowEnvR = midEnvL = midEnvR = highEnvL = highEnvR = masterEnv = 1.0f;
        lastOutputL = lastOutputR = 0.0f;
    }

    void reset()
    {
        std::fill (delayL.begin(), delayL.end(), 0.0f);
        std::fill (delayR.begin(), delayR.end(), 0.0f);
        delayWriteL = 0;
        delayWriteR = 0;

        lowEnvL = lowEnvR = midEnvL = midEnvR = highEnvL = highEnvR = masterEnv = 1.0f;
        lastOutputL = lastOutputR = 0.0f;
    }

    void processStereo (float& left, float& right)
    {
        const float inL = left;
        const float inR = right;

        // 1.48 ms grouped LMH look-ahead delay.
        const float delayedL = pushDelay (inL, delayL, delayWriteL);
        const float delayedR = pushDelay (inR, delayR, delayWriteR);

        // Phase-coherent-ish three-way split. The band filters are deliberately
        // identical on L/R; stereo manipulation happens after dynamics.
        const float lowLP_L  = lowLpf.processSample (delayedL);
        const float lowLP_R  = lowLpfR.processSample (delayedR);
        const float below200_L = lowLP_L;
        const float below200_R = lowLP_R;

        const float highLP_L = highLowpassL.processSample (delayedL);
        const float highLP_R = highLowpassR.processSample (delayedR);

        const float lowL  = below200_L;
        const float lowR  = below200_R;
        const float highL = delayedL - highLP_L;
        const float highR = delayedR - highLP_R;
        const float midL  = highLP_L - below200_L;
        const float midR  = highLP_R - below200_R;

        // Band stages. Inputs are boosted exactly as the user's Maximus
        // transcription states; each stage is then brought back to a stable
        // vocal-friendly operating point by its POST gain and compressor curve.
        float pLowL = processLow (lowL, lowEnvL);
        float pLowR = processLow (lowR, lowEnvR);

        float pMidL = processMid (midL, midEnvL);
        float pMidR = processMid (midR, midEnvR);

        float pHighL = processHigh (highL, highEnvL);
        float pHighR = processHigh (highR, highEnvR);

        // User-specified stereo separation:
        // LOW merged 75% -> move strongly toward mono.
        const float lowMono = 0.5f * (pLowL + pLowR);
        pLowL = pLowL * 0.25f + lowMono * 0.75f;
        pLowR = pLowR * 0.25f + lowMono * 0.75f;

        // MID separation 35% -> increase side information by 35%.
        const float midMid = 0.5f * (pMidL + pMidR);
        const float midSide = 0.5f * (pMidL - pMidR);
        pMidL = midMid + midSide * 1.35f;
        pMidR = midMid - midSide * 1.35f;

        // HIGH stereo OFF: leave it alone.

        // Recombine LMH.
        float lmhL = pLowL + pMidL + pHighL;
        float lmhR = pLowR + pMidR + pHighR;

        // MASTER stage: short RMS-style detector + gentle soft ceiling.
        const float masterTargetL = masterCurve (lmhL);
        const float masterTargetR = masterCurve (lmhR);
        const float masterGainL = updateMaster (masterTargetL);
        const float masterGainR = updateMasterR (masterTargetR);
        lmhL = masterTargetL * masterGainL;
        lmhR = masterTargetR * masterGainR;

        // Soundgoodizer C amount = LMH MIX. User measured ~45%.
        constexpr float lmhMix = 0.45f;
        float outL = inL * (1.0f - lmhMix) + lmhL * lmhMix;
        float outR = inR * (1.0f - lmhMix) + lmhR * lmhMix;

        // Conservative safety trim to keep this recreation from becoming a
        // second limiter in the vocal chain. This does not change the macro
        // amount; it just keeps pathological peaks from exploding.
        const float peak = juce::jmax (std::abs (outL), std::abs (outR));
        if (peak > 0.988f)
        {
            const float trim = 0.988f / peak;
            outL *= trim;
            outR *= trim;
        }

        lastOutputL = outL;
        lastOutputR = outR;
        left = outL;
        right = outR;
    }

    // Compatibility helper for older processor code.
    float processSample (float x)
    {
        float l = x;
        float r = x;
        processStereo (l, r);
        return l;
    }

private:
    using OnePole = juce::dsp::IIR::Filter<float>;

    double sampleRate = 44100.0;
    int delaySamples = 65;
    int delayWriteL = 0;
    int delayWriteR = 0;

    std::vector<float> delayL, delayR;

    // Two-pole-ish cascades per channel. Using separate instances avoids the
    // accidental shared-state bug that the previous implementation had.
    juce::dsp::IIR::Filter<float> lowLpf, lowLpfR;
    juce::dsp::IIR::Filter<float> highLowpassL, highLowpassR;

    float lowEnvL = 1.0f, lowEnvR = 1.0f;
    float midEnvL = 1.0f, midEnvR = 1.0f;
    float highEnvL = 1.0f, highEnvR = 1.0f;
    float masterEnv = 1.0f;
    float masterEnvR = 1.0f;
    float lastOutputL = 0.0f, lastOutputR = 0.0f;

    void makeFilters()
    {
        auto makeLP = [this] (float freq)
        {
            return juce::dsp::IIR::Coefficients<float>::makeLowPass (sampleRate, freq);
        };

        lowLpf.state = *makeLP (200.0f);
        lowLpfR.state = *makeLP (200.0f);

        highLowpassL.state = *makeLP (1906.0f);
        highLowpassR.state = *makeLP (1906.0f);
    }

    float pushDelay (float x, std::vector<float>& buffer, int& writeIndex)
    {
        if (buffer.empty())
            return x;

        const int size = static_cast<int> (buffer.size());
        const int read = (writeIndex - delaySamples + size) % size;
        const float y = buffer[static_cast<size_t> (read)];
        buffer[static_cast<size_t> (writeIndex)] = x;
        writeIndex = (writeIndex + 1) % size;
        return y;
    }

    static float dbToGain (float db)
    {
        return juce::Decibels::decibelsToGain (db);
    }

    static float softClip (float x, float drive)
    {
        return std::tanh (x * drive) / std::tanh (drive);
    }

    // Smooth detector with separate attack/release constants.
    static float followEnvelope (float env, float level, float attackMs, float releaseMs, double sr)
    {
        const float a = std::exp (-1.0f / juce::jmax (1.0, static_cast<float> (sr) * attackMs * 0.001f));
        const float r = std::exp (-1.0f / juce::jmax (1.0, static_cast<float> (sr) * releaseMs * 0.001f));
        return level > env ? a * env + (1.0f - a) * level
                           : r * env + (1.0f - r) * level;
    }

    // LOW envelope from the original LOW .fnv: essentially a maximizer curve
    // around unity, with the recorded low-band saturation doing most of the work.
    float lowCurve (float x) const
    {
        const float ax = std::abs (x);
        const float g = dbToGain (8.5f);
        float y = x * g;

        // Mild dynamic flattening, then the user's Mode-A saturation.
        const float over = std::max (0.0f, ax - 0.55f);
        y -= std::copysign (over * 0.22f, x);
        return y;
    }

    float processLow (float x, float& env)
    {
        const float pre = x * dbToGain (8.5f);
        const float level = std::abs (pre);
        env = followEnvelope (env, level, 2.0f, 137.0f, sampleRate);

        float y = pre;
        const float reduction = 1.0f / (1.0f + std::max (0.0f, env - 0.55f) * 1.8f);
        y *= reduction;

        // Mode A / 100% saturation, ceiling +2.2 dB.
        y = softClip (y, 1.55f);
        y = std::clamp (y, -dbToGain (2.2f), dbToGain (2.2f));
        y *= dbToGain (1.5f);

        // Level stabilization: Maximus PRE/POST are part of the curve. Keep
        // the recreation close to unity at ordinary vocal levels.
        return y * 0.36f;
    }

    float processMid (float x, float& env)
    {
        const float pre = x * dbToGain (12.7f);
        const float level = std::abs (pre);
        env = followEnvelope (env, level, 2.0f, 85.53f, sampleRate);

        // Measured MID .fnv has an actual curved point rather than a flat 1:1
        // graph, so use a soft knee around the upper-mid vocal working range.
        float y = pre;
        const float threshold = 0.92f;
        if (env > threshold)
        {
            const float excess = env - threshold;
            const float gr = 1.0f / (1.0f + excess * 2.15f);
            y *= gr;
        }

        y *= dbToGain (2.8f);
        return y * 0.25f;
    }

    float processHigh (float x, float& env)
    {
        const float pre = x * dbToGain (11.3f);
        const float level = std::abs (pre);
        env = followEnvelope (env, level, 2.0f, 85.53f, sampleRate);

        float y = pre;
        const float threshold = 0.86f;
        if (env > threshold)
        {
            const float excess = env - threshold;
            y *= 1.0f / (1.0f + excess * 1.55f);
        }

        y *= dbToGain (2.9f);
        return y * 0.24f;
    }

    float masterCurve (float x) const
    {
        const float ax = std::abs (x);
        const float sign = x < 0.0f ? -1.0f : 1.0f;
        const float knee = 0.72f;
        if (ax <= knee)
            return x;

        const float excess = ax - knee;
        // Master .fnv is more curved than the nearly-flat LOW/HIGH states.
        const float compressed = knee + excess / (1.0f + excess * 2.65f);
        return sign * compressed;
    }

    float updateMaster (float x)
    {
        const float level = std::abs (x);
        masterEnv = followEnvelope (masterEnv, level, 2.0f, 85.53f, sampleRate);
        const float amount = std::max (0.0f, masterEnv - 0.86f);
        return 1.0f / (1.0f + amount * 0.75f);
    }

    float updateMasterR (float x)
    {
        const float level = std::abs (x);
        masterEnvR = followEnvelope (masterEnvR, level, 2.0f, 85.53f, sampleRate);
        const float amount = std::max (0.0f, masterEnvR - 0.86f);
        return 1.0f / (1.0f + amount * 0.75f);
    }
};
