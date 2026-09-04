#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>

/**
    VocalParametricEQ7

    Реконструкция 7-полосной кривой
    Fruity Parametric EQ 2.

    Значения восстановлены по форме
    исходной кривой.

    7 узлов:

    C1 = 30 Hz
    C2 = 100 Hz
    C3 = 250 Hz
    C4 = 800 Hz
    C5 = 2 kHz
    C6 = 5 kHz
    C7 = 9 kHz

    tone:
    -50 = теплее
    +50 = ярче
*/

class VocalParametricEQ7
{
public:

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;

        // Каждый из 7 фильтров имеет
        // отдельное состояние для L и R.
        for (auto& band : bands)
        {
            for (auto& filter : band)
                filter.prepare (spec);
        }

        update (0.0f);
    }


    void reset()
    {
        for (auto& band : bands)
        {
            for (auto& filter : band)
                filter.reset();
        }
    }


    /**
        tone: -50..+50

        -50 = теплее
        +50 = ярче
    */
    void update (float tone)
    {
        const float t =
            juce::jlimit (
                -1.0f,
                1.0f,
                tone / 50.0f
            );


        struct Node
        {
            float freq;
            float gainDb;
            float q;
        };


        // Базовая кривая EQ.
        //
        // Эти значения соответствуют
        // предыдущей реконструкции кривой.

        Node nodes[7] =
        {
            // C1
            { 30.0f,
             -18.0f,
               0.6f },

            // C2
            { 100.0f,
                2.0f,
                0.9f },

            // C3
            { 250.0f,
                1.0f,
                1.0f },

            // C4
            { 800.0f,
                6.0f,
                1.1f },

            // C5
            { 2000.0f,
               -2.0f,
                1.2f },

            // C6
            { 5000.0f,
                6.0f,
                1.0f },

            // C7
            { 9000.0f,
                9.0f,
                0.7f }
        };


        // -----------------------------------------
        // Крутилка "Теплее -- Ярче"
        // -----------------------------------------

        // Ярче:
        // немного уменьшаем низ
        // и усиливаем верх.

        nodes[0].gainDb += -t * 3.0f;
        nodes[1].gainDb += -t * 1.5f;

        nodes[5].gainDb +=  t * 2.0f;
        nodes[6].gainDb +=  t * 3.0f;


        // -----------------------------------------
        // Создаём коэффициенты фильтров
        // -----------------------------------------

        for (int i = 0; i < 7; ++i)
        {
            // C1 — high-pass.
            //
            // Gain для него не используется.
            if (i == 0)
            {
                auto coefficients =
                    juce::dsp::IIR::Coefficients<float>::makeHighPass (
                        sampleRate,
                        nodes[i].freq,
                        nodes[i].q
                    );

                for (auto& filter : bands[i])
                    filter.coefficients = coefficients;
            }
            else
            {
                // C2-C7 — peaking EQ.
                auto coefficients =
                    juce::dsp::IIR::Coefficients<float>::makePeakFilter (
                        sampleRate,
                        nodes[i].freq,
                        nodes[i].q,
                        juce::Decibels::decibelsToGain (
                            nodes[i].gainDb
                        )
                    );

                for (auto& filter : bands[i])
                    filter.coefficients = coefficients;
            }
        }
    }


    /**
        Обработка одного сэмпла.

        channel:
        0 = Left
        1 = Right
    */
    float processSample (int channel, float x)
    {
        const int ch =
            juce::jlimit (0, 1, channel);


        // Строго последовательно
        // проходим через все 7 полос.

        for (auto& band : bands)
        {
            x = band[ch].processSample (x);
        }


        return x;
    }


private:

    double sampleRate = 44100.0;


    /**
        7 полос × 2 канала.

        bands[0][0] = C1 Left
        bands[0][1] = C1 Right

        bands[1][0] = C2 Left
        bands[1][1] = C2 Right

        ...

        bands[6][0] = C7 Left
        bands[6][1] = C7 Right
    */
    std::array<
        std::array<
            juce::dsp::IIR::Filter<float>,
            2
        >,
        7
    > bands;
};
