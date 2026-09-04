#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>

/**
    FreshAirStage

    Эмуляция воздушного эксайтера:
    - Low band: ~3.5 kHz+
    - High band: ~10 kHz+

    Базовые значения:
    Low  = +8 / 10
    High = +8 / 10

    tone:
    -50 = теплее
    +50 = ярче

    Важно:
    Каждый канал имеет собственное состояние IIR-фильтра.
*/

class FreshAirStage
{
public:

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;

        for (auto& filter : lowBandFilter)
            filter.prepare (spec);

        for (auto& filter : highBandFilter)
            filter.prepare (spec);

        auto lowCoefficients =
            juce::dsp::IIR::Coefficients<float>::makeHighPass (
                sampleRate,
                3500.0f,
                0.7f
            );

        auto highCoefficients =
            juce::dsp::IIR::Coefficients<float>::makeHighPass (
                sampleRate,
                10000.0f,
                0.7f
            );

        for (auto& filter : lowBandFilter)
            filter.coefficients = lowCoefficients;

        for (auto& filter : highBandFilter)
            filter.coefficients = highCoefficients;

        update (0.0f);
    }


    void reset()
    {
        for (auto& filter : lowBandFilter)
            filter.reset();

        for (auto& filter : highBandFilter)
            filter.reset();
    }


    /**
        tone: -50..+50

        -50 = теплее
        +50 = ярче
    */
    void update (float tone)
    {
        const float t =
            juce::jlimit (-1.0f, 1.0f, tone / 50.0f);

        // База +8 из 10
        //
        // Теплее:
        // немного уменьшаем эксайтер
        //
        // Ярче:
        // немного увеличиваем

        lowAmount =
            juce::jlimit (
                0.0f,
                1.0f,
                0.8f + t * 0.15f
            );

        highAmount =
            juce::jlimit (
                0.0f,
                1.0f,
                0.8f + t * 0.15f
            );
    }


    float processSample (int channel, float x)
    {
        // Поддерживаем stereo L/R.
        const int ch = juce::jlimit (0, 1, channel);

        // Выделяем верхнюю часть спектра.
        const float lowBand =
            lowBandFilter[ch].processSample (x);

        const float highBand =
            highBandFilter[ch].processSample (x);

        // Гармоническое насыщение.
        const float lowHarmonics =
            exciter (lowBand)
            * lowAmount
            * 0.6f;

        const float highHarmonics =
            exciter (highBand)
            * highAmount
            * 0.6f;

        // Подмешиваем обратно к исходному сигналу.
        return x
            + lowHarmonics
            + highHarmonics;
    }


private:

    double sampleRate = 44100.0;

    float lowAmount  = 0.8f;
    float highAmount = 0.8f;


    // Отдельное состояние фильтра для L и R.
    std::array<
        juce::dsp::IIR::Filter<float>,
        2
    > lowBandFilter;


    std::array<
        juce::dsp::IIR::Filter<float>,
        2
    > highBandFilter;


    /**
        Ассиметричная сатурация.

        Создаёт дополнительные гармоники,
        что даёт эффект "air / presence".
    */
    static float exciter (float x)
    {
        const float positive =
            std::tanh (x * 3.0f);

        const float asymmetric =
            0.3f
            * std::tanh (x * x * 3.0f)
            * (x < 0.0f ? -1.0f : 1.0f);

        return positive - asymmetric;
    }
};
