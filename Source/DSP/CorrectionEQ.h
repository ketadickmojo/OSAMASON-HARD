#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>

/**
    CorrectionEQ — первый блок цепи.

    Базовые значения:
    90 Hz  -11 dB
    1.5 kHz +3.3 dB
    8 kHz  +9.5 dB

    tone: -50..+50
    отрицательное значение = теплее
    положительное значение = ярче
*/

class CorrectionEQ
{
public:

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;

        for (auto& f : lowShelf)
            f.prepare (spec);

        for (auto& f : bell)
            f.prepare (spec);

        for (auto& f : highShelf)
            f.prepare (spec);

        update (0.0f);
    }

    void reset()
    {
        for (auto& f : lowShelf)
            f.reset();

        for (auto& f : bell)
            f.reset();

        for (auto& f : highShelf)
            f.reset();
    }

    // tone: -50..+50
    void update (float tone)
    {
        const float toneNorm =
            juce::jlimit (-1.0f, 1.0f, tone / 50.0f);

        // Базовые значения
        // -11 dB @ 90 Hz
        // +3.3 dB @ 1.5 kHz
        // +9.5 dB @ 8 kHz

        const float lowGainDb =
            -11.0f + toneNorm * -4.0f;

        const float bellGainDb =
             3.3f + toneNorm *  1.0f;

        const float highGainDb =
             9.5f + toneNorm *  4.0f;

        auto lowCoefficients =
            juce::dsp::IIR::Coefficients<float>::makeLowShelf (
                sampleRate,
                90.0f,
                0.71f,
                juce::Decibels::decibelsToGain (lowGainDb));

        auto bellCoefficients =
            juce::dsp::IIR::Coefficients<float>::makePeakFilter (
                sampleRate,
                1500.0f,
                1.0f,
                juce::Decibels::decibelsToGain (bellGainDb));

        auto highCoefficients =
            juce::dsp::IIR::Coefficients<float>::makeHighShelf (
                sampleRate,
                8000.0f,
                0.71f,
                juce::Decibels::decibelsToGain (highGainDb));

        // Один набор коэффициентов для обоих каналов
        for (auto& f : lowShelf)
            f.coefficients = lowCoefficients;

        for (auto& f : bell)
            f.coefficients = bellCoefficients;

        for (auto& f : highShelf)
            f.coefficients = highCoefficients;
    }

    float processSample (int channel, float x)
    {
        // Поддерживаем stereo.
        const int ch = juce::jlimit (0, 1, channel);

        x = lowShelf[ch].processSample (x);
        x = bell[ch].processSample (x);
        x = highShelf[ch].processSample (x);

        return x;
    }

private:

    double sampleRate = 44100.0;

    // Отдельное состояние фильтра для L и R.
    std::array<juce::dsp::IIR::Filter<float>, 2> lowShelf;
    std::array<juce::dsp::IIR::Filter<float>, 2> bell;
    std::array<juce::dsp::IIR::Filter<float>, 2> highShelf;
};
