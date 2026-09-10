#pragma once

#include <juce_dsp/juce_dsp.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <vector>
#include <algorithm>

/**
    SoundgoodizerC

    Собственная реконструкция Soundgoodizer C через
    Maximus-style 3-band dynamics architecture.

    Эталон пользователя:

    LOW
        PRE      +8.5 dB
        POST     +1.5 dB
        ATT      2 ms
        REL      137 ms
        SUSTAIN  10 ms
        THRES    100% SATURATION, MODE A
        CEIL     +2.2 dB
        ATT CURVE 2
        REL CURVE 3
        REL2 CURVE 2

    MID
        PRE      +12.7 dB
        POST     +2.8 dB
        ATT      2 ms
        REL      85.53 ms
        SUSTAIN  3.31 ms
        THRES    NO SATURATION
        CEIL     0 dB
        ATT CURVE 2
        REL CURVE 3
        REL2 CURVE 2
        STEREO SEPARATION = 35%

    HIGH
        PRE      +11.3 dB
        POST     +2.9 dB
        ATT      2 ms
        REL      85.53 ms
        SUSTAIN  2.18 ms
        THRES    NO SATURATION
        CEIL     0 dB
        ATT CURVE 2
        REL CURVE 3
        REL2 CURVE 2

    MASTER
        PRE      0 dB
        POST     0 dB
        ATT      2 ms
        REL      85.53 ms
        SUSTAIN  10 ms
        THRES    NO SATURATION
        CEIL     0 dB
        ATT CURVE 2
        REL CURVE 3
        REL2     60.16 ms / CURVE 2

    CROSSOVERS
        LOW/MID  = 200 Hz
        MID/HIGH = 1906 Hz

    LMH DEL = 1.48 ms
    LMH MIX  = controlled by Soundgoodizer amount

    IMPORTANT:
    Soundgoodizer's big knob corresponds to Maximus LMH MIX.
    The original Soundgoodizer C preset itself is fixed;
    the amount knob mixes dry input with processed LMH output.
*/

class SoundgoodizerC
{
public:

    // ============================================================
    // PREPARE
    // ============================================================

    void prepare (double sampleRate)
    {
        fs = sampleRate;

        if (fs <= 0.0)
            fs = 44100.0;

        prepareSplitFilters();
        prepareLookahead();

        update (45.0f);
        reset();
    }

    // ============================================================
    // RESET
    // ============================================================

    void reset()
    {
        for (auto& channel : channels)
            channel.reset();

        for (auto& filter : low200A)
            filter.reset();

        for (auto& filter : low200B)
            filter.reset();

        for (auto& filter : low1906A)
            filter.reset();

        for (auto& filter : low1906B)
            filter.reset();

        for (auto& filter : high1906A)
            filter.reset();

        for (auto& filter : high1906B)
            filter.reset();

        for (auto& delay : lookahead)
        {
            std::fill (
                delay.begin(),
                delay.end(),
                0.0f);
        }

        lookaheadWrite = { 0, 0 };
    }

    // ============================================================
    // SOUNDGOODIZER AMOUNT
    //
    // This is the equivalent of LMH MIX.
    //
    // 0%   = dry
    // 45%  = your preset
    // 100% = fully processed LMH
    // ============================================================

    void update (float amount)
    {
        mix =
            std::clamp (
                amount,
                0.0f,
                100.0f)
            / 100.0f;
    }

    // ============================================================
    // PROCESS SAMPLE
    // ============================================================

    float processSample (
        int channel,
        float x)
    {
        const std::size_t ch =
            static_cast<std::size_t> (
                std::clamp (
                    channel,
                    0,
                    1));

        // --------------------------------------------------------
        // 1. LMH LOOK-AHEAD DELAY
        //
        // Maximus uses LMH delay to make the dynamics react
        // before the corresponding audio transient is output.
        // --------------------------------------------------------

        const float delayed =
            processLookahead (
                ch,
                x);

        // --------------------------------------------------------
        // 2. THREE-BAND SPLIT
        //
        // Low:  < 200 Hz
        // Mid:  200 Hz - 1906 Hz
        // High: > 1906 Hz
        // --------------------------------------------------------

        float low =
            low200A[ch].processSample (
                delayed);

        low =
            low200B[ch].processSample (
                low);

        float lowMid =
            low1906A[ch].processSample (
                delayed);

        lowMid =
            low1906B[ch].processSample (
                lowMid);

        float high =
            high1906A[ch].processSample (
                delayed);

        high =
            high1906B[ch].processSample (
                high);

        // Mid is the band between the two crossovers.
        float mid =
            lowMid - low;

        // --------------------------------------------------------
        // 3. LOW BAND
        // --------------------------------------------------------

        low =
            processBand (
                low,
                channels[ch].low,
                lowPreGain,
                lowPostGain,
                lowAttackMs,
                lowReleaseMs,
                lowSustainMs,
                lowCeilingDb,
                true,
                lowSaturationAmount);

        // --------------------------------------------------------
        // 4. MID BAND
        // --------------------------------------------------------

        mid =
            processBand (
                mid,
                channels[ch].mid,
                midPreGain,
                midPostGain,
                midAttackMs,
                midReleaseMs,
                midSustainMs,
                midCeilingDb,
                false,
                0.0f);

        // --------------------------------------------------------
        // 5. HIGH BAND
        // --------------------------------------------------------

        high =
            processBand (
                high,
                channels[ch].high,
                highPreGain,
                highPostGain,
                highAttackMs,
                highReleaseMs,
                highSustainMs,
                highCeilingDb,
                false,
                0.0f);

        // --------------------------------------------------------
        // 6. RECOMBINE LOW / MID / HIGH
        // --------------------------------------------------------

        float processed =
            low + mid + high;

        // --------------------------------------------------------
        // 7. MASTER STAGE
        //
        // Approximation of the Maximus MASTER envelope.
        // --------------------------------------------------------

        processed =
            processMaster (
                processed,
                channels[ch].master);

        // --------------------------------------------------------
        // 8. LMH MIX
        //
        // Soundgoodizer's big knob is this blend.
        // --------------------------------------------------------

        return
            delayed * (1.0f - mix)
            + processed * mix;
    }

private:

    // ============================================================
    // CONSTANTS
    // ============================================================

    static constexpr float kLowCrossoverHz =
        200.0f;

    static constexpr float kHighCrossoverHz =
        1906.0f;

    static constexpr float kLookaheadMs =
        1.48f;

    // ============================================================
    // BAND PARAMETERS
    // ============================================================

    // LOW
    static constexpr float lowPreGain =
        8.5f;

    static constexpr float lowPostGain =
        1.5f;

    static constexpr float lowAttackMs =
        2.0f;

    static constexpr float lowReleaseMs =
        137.0f;

    static constexpr float lowSustainMs =
        10.0f;

    static constexpr float lowCeilingDb =
        2.2f;

    // 100% saturation / Mode A.
    static constexpr float lowSaturationAmount =
        1.0f;

    // MID
    static constexpr float midPreGain =
        12.7f;

    static constexpr float midPostGain =
        2.8f;

    static constexpr float midAttackMs =
        2.0f;

    static constexpr float midReleaseMs =
        85.53f;

    static constexpr float midSustainMs =
        3.31f;

    static constexpr float midCeilingDb =
        0.0f;

    // HIGH
    static constexpr float highPreGain =
        11.3f;

    static constexpr float highPostGain =
        2.9f;

    static constexpr float highAttackMs =
        2.0f;

    static constexpr float highReleaseMs =
        85.53f;

    static constexpr float highSustainMs =
        2.18f;

    static constexpr float highCeilingDb =
        0.0f;

    // MASTER
    static constexpr float masterPreGain =
        0.0f;

    static constexpr float masterPostGain =
        0.0f;

    static constexpr float masterAttackMs =
        2.0f;

    static constexpr float masterReleaseMs =
        85.53f;

    static constexpr float masterSustainMs =
        10.0f;

    static constexpr float masterCeilingDb =
        0.0f;

    static constexpr float masterRel2Ms =
        60.16f;

    // ============================================================
    // BAND STATE
    // ============================================================

    struct BandState
    {
        float envelopeDb = -120.0f;
        float gainReductionDb = 0.0f;

        void reset()
        {
            envelopeDb = -120.0f;
            gainReductionDb = 0.0f;
        }
    };

    struct ChannelState
    {
        BandState low;
        BandState mid;
        BandState high;
        BandState master;

        void reset()
        {
            low.reset();
            mid.reset();
            high.reset();
            master.reset();
        }
    };

    // ============================================================
    // STATE
    // ============================================================

    double fs = 44100.0;

    float mix = 0.45f;

    std::array<ChannelState, 2> channels;

    // ============================================================
    // SPLIT FILTERS
    // ============================================================

    //
    // 4th-order Low Pass at 200 Hz
    //
    std::array<
        juce::dsp::IIR::Filter<float>,
        2> low200A;

    std::array<
        juce::dsp::IIR::Filter<float>,
        2> low200B;

    //
    // 4th-order Low Pass at 1906 Hz
    //
    std::array<
        juce::dsp::IIR::Filter<float>,
        2> low1906A;

    std::array<
        juce::dsp::IIR::Filter<float>,
        2> low1906B;

    //
    // 4th-order High Pass at 1906 Hz
    //
    std::array<
        juce::dsp::IIR::Filter<float>,
        2> high1906A;

    std::array<
        juce::dsp::IIR::Filter<float>,
        2> high1906B;

    // ============================================================
    // LOOKAHEAD
    // ============================================================

    std::array<
        std::vector<float>,
        2> lookahead;

    std::array<int, 2> lookaheadWrite { 0, 0 };

    int lookaheadSamples = 1;

    // ============================================================
    // PREPARE FILTERS
    // ============================================================

    void prepareSplitFilters()
    {
        juce::dsp::ProcessSpec spec;

        spec.sampleRate =
            fs;

        spec.maximumBlockSize =
            1;

        spec.numChannels =
            1;

        for (auto& filter : low200A)
            filter.prepare (spec);

        for (auto& filter : low200B)
            filter.prepare (spec);

        for (auto& filter : low1906A)
            filter.prepare (spec);

        for (auto& filter : low1906B)
            filter.prepare (spec);

        for (auto& filter : high1906A)
            filter.prepare (spec);

        for (auto& filter : high1906B)
            filter.prepare (spec);

        auto low200 =
            juce::dsp::IIR::Coefficients<float>::makeLowPass (
                fs,
                kLowCrossoverHz,
                0.70710678f);

        auto low1906 =
            juce::dsp::IIR::Coefficients<float>::makeLowPass (
                fs,
                kHighCrossoverHz,
                0.70710678f);

        auto high1906 =
            juce::dsp::IIR::Coefficients<float>::makeHighPass (
                fs,
                kHighCrossoverHz,
                0.70710678f);

        for (auto& filter : low200A)
            filter.coefficients = low200;

        for (auto& filter : low200B)
            filter.coefficients = low200;

        for (auto& filter : low1906A)
            filter.coefficients = low1906;

        for (auto& filter : low1906B)
            filter.coefficients = low1906;

        for (auto& filter : high1906A)
            filter.coefficients = high1906;

        for (auto& filter : high1906B)
            filter.coefficients = high1906;
    }

    // ============================================================
    // PREPARE LOOKAHEAD
    // ============================================================

    void prepareLookahead()
    {
        lookaheadSamples =
            juce::jmax (
                1,
                static_cast<int> (
                    std::round (
                        fs
                        * kLookaheadMs
                        * 0.001)));

        for (auto& delay : lookahead)
        {
            delay.assign (
                static_cast<std::size_t> (
                    lookaheadSamples),
                0.0f);
        }

        lookaheadWrite = { 0, 0 };
    }

    // ============================================================
    // LOOKAHEAD PROCESS
    // ============================================================

    float processLookahead (
        std::size_t channel,
        float input)
    {
        if (lookahead.empty()
            || lookahead[channel].empty())
        {
            return input;
        }

        auto& delay =
            lookahead[channel];

        const std::size_t size =
            delay.size();

        const std::size_t write =
            static_cast<std::size_t> (
                lookaheadWrite[channel]);

        const float output =
            delay[write];

        delay[write] =
            input;

        ++lookaheadWrite[channel];

        if (lookaheadWrite[channel]
            >= static_cast<int> (size))
        {
            lookaheadWrite[channel] = 0;
        }

        return output;
    }

    // ============================================================
    // DB HELPERS
    // ============================================================

    static float dbToLinear (float db)
    {
        return
            juce::Decibels::decibelsToGain (
                db);
    }

    static float linearToDb (float linear)
    {
        return
            juce::Decibels::gainToDecibels (
                juce::jmax (
                    linear,
                    1.0e-9f));
    }

    // ============================================================
    // COEFFICIENT
    // ============================================================

    float coeffForMs (float ms) const
    {
        if (ms <= 0.001f)
            return 1.0f;

        const float seconds =
            ms * 0.001f;

        return
            1.0f
            - std::exp (
                -1.0f
                /
                static_cast<float> (
                    fs * seconds));
    }

    // ============================================================
    // MAXIMUS-STYLE COMPRESSION CURVE
    //
    // Curve 2:
    // relatively soft knee / musical transition.
    //
    // Curve 3:
    // stronger transition / more aggressive control.
    //
    // Since the original Maximus curve itself is not exposed as
    // a simple ratio value, this is an approximation of the
    // nonlinear transfer shape.
    // ============================================================

    static float compressionCurve (
        float levelDb,
        float ceilingDb,
        float kneeDb,
        float strength)
    {
        if (levelDb <= ceilingDb - kneeDb)
            return levelDb;

        const float x =
            levelDb
            - (ceilingDb - kneeDb);

        const float normalized =
            juce::jlimit (
                0.0f,
                1.0f,
                x / (2.0f * kneeDb));

        //
        // Smooth S-shaped knee.
        //
        const float shaped =
            normalized
            * normalized
            * (3.0f - 2.0f * normalized);

        const float excess =
            levelDb
            - ceilingDb;

        const float compressedExcess =
            excess
            * (1.0f - strength
               * shaped);

        return
            ceilingDb
            + compressedExcess;
    }

    // ============================================================
    // BAND PROCESSOR
    // ============================================================

    float processBand (
        float input,
        BandState& state,
        float preGainDb,
        float postGainDb,
        float attackMs,
        float releaseMs,
        float sustainMs,
        float ceilingDb,
        bool useSaturation,
        float saturationAmount)
    {
        float x =
            input
            * dbToLinear (
                preGainDb);

        const float inputDb =
            linearToDb (
                std::abs (x));

        // --------------------------------------------------------
        // Envelope
        // --------------------------------------------------------

        const float attackCoeff =
            coeffForMs (
                attackMs);

        const float releaseCoeff =
            coeffForMs (
                releaseMs);

        if (inputDb >
            state.envelopeDb)
        {
            state.envelopeDb +=
                (inputDb
                 - state.envelopeDb)
                * attackCoeff;
        }
        else
        {
            state.envelopeDb +=
                (inputDb
                 - state.envelopeDb)
                * releaseCoeff;
        }

        // --------------------------------------------------------
        // Sustain acts as an additional memory component.
        //
        // Short sustain values are kept subtle so that the
        // transient envelope remains the dominant detector.
        // --------------------------------------------------------

        const float sustainCoeff =
            coeffForMs (
                juce::jmax (
                    0.1f,
                    sustainMs));

        const float heldEnvelope =
            state.envelopeDb
            * (1.0f - sustainCoeff)
            + inputDb
            * sustainCoeff;

        state.envelopeDb =
            heldEnvelope;

        // --------------------------------------------------------
        // Maximus-style nonlinear gain reduction.
        // --------------------------------------------------------

        constexpr float kneeDb =
            4.0f;

        constexpr float curveStrength =
            0.85f;

        const float targetLevel =
            compressionCurve (
                state.envelopeDb,
                ceilingDb,
                kneeDb,
                curveStrength);

        const float gainReductionDb =
            juce::jmin (
                0.0f,
                targetLevel
                - state.envelopeDb);

        state.gainReductionDb =
            gainReductionDb;

        const float gain =
            dbToLinear (
                gainReductionDb);

        float processed =
            x * gain;

        // --------------------------------------------------------
        // LOW BAND SATURATION
        //
        // Mode A approximation.
        // Saturation is intentionally applied after the
        // compression envelope and before POST gain.
        // --------------------------------------------------------

        if (useSaturation
            && saturationAmount > 0.0f)
        {
            const float drive =
                1.8f
                + saturationAmount * 2.6f;

            const float saturated =
                std::tanh (
                    processed * drive);

            processed =
                processed
                * (1.0f
                   - saturationAmount)
                +
                saturated
                * saturationAmount;
        }

        // --------------------------------------------------------
        // POST
        // --------------------------------------------------------

        processed *=
            dbToLinear (
                postGainDb);

        // --------------------------------------------------------
        // Soft ceiling.
        //
        // LOW ceiling = +2.2 dB
        // MID/HIGH ceiling = 0 dB
        // --------------------------------------------------------

        const float ceilingLinear =
            dbToLinear (
                ceilingDb);

        if (std::abs (processed)
            > ceilingLinear)
        {
            const float sign =
                processed < 0.0f
                    ? -1.0f
                    : 1.0f;

            const float excess =
                std::abs (processed)
                - ceilingLinear;

            processed =
                sign
                * (ceilingLinear
                   + std::tanh (
                       excess)
                   * 0.25f);
        }

        return processed;
    }

    // ============================================================
    // MASTER
    // ============================================================

    float processMaster (
        float input,
        BandState& state)
    {
        float x =
            input
            * dbToLinear (
                masterPreGain);

        const float inputDb =
            linearToDb (
                std::abs (x));

        const float attackCoeff =
            coeffForMs (
                masterAttackMs);

        const float releaseCoeff =
            coeffForMs (
                masterReleaseMs);

        if (inputDb >
            state.envelopeDb)
        {
            state.envelopeDb +=
                (inputDb
                 - state.envelopeDb)
                * attackCoeff;
        }
        else
        {
            state.envelopeDb +=
                (inputDb
                 - state.envelopeDb)
                * releaseCoeff;
        }

        // --------------------------------------------------------
        // Master limiter / compressor.
        //
        // Harder curve than the band stages.
        // --------------------------------------------------------

        constexpr float kneeDb =
            1.0f;

        constexpr float strength =
            1.0f;

        const float targetLevel =
            compressionCurve (
                state.envelopeDb,
                masterCeilingDb,
                kneeDb,
                strength);

        const float reductionDb =
            juce::jmin (
                0.0f,
                targetLevel
                - state.envelopeDb);

        state.gainReductionDb =
            reductionDb;

        x *=
            dbToLinear (
                reductionDb);

        // --------------------------------------------------------
        // MASTER REL2 damping.
        //
        // Used as a slower release tail so that the gain
        // doesn't snap back too aggressively after transients.
        // --------------------------------------------------------

        const float rel2Coeff =
            coeffForMs (
                masterRel2Ms);

        const float smoothedGR =
            state.gainReductionDb
            * (1.0f - rel2Coeff)
            + reductionDb
            * rel2Coeff;

        state.gainReductionDb =
            smoothedGR;

        x *=
            dbToLinear (
                smoothedGR
                - reductionDb);

        // --------------------------------------------------------
        // POST
        // --------------------------------------------------------

        x *=
            dbToLinear (
                masterPostGain);

        // --------------------------------------------------------
        // FINAL CEILING
        // --------------------------------------------------------

        const float ceiling =
            dbToLinear (
                masterCeilingDb);

        if (std::abs (x)
            > ceiling)
        {
            x =
                std::copysign (
                    ceiling,
                    x);
        }

        return x;
    }
};
