#pragma once

#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cstddef>

class VocalParametricEQ7
{
public:

    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;

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
            float frequency;
            float gainDb;
            float q;
        };

        Node nodes[7] =
        {
            { 30.0f,   -18.0f, 0.6f },
            { 100.0f,    2.0f, 0.9f },
            { 250.0f,    1.0f, 1.0f },
            { 800.0f,    6.0f, 1.1f },
            { 2000.0f,  -2.0f, 1.2f },
            { 5000.0f,   6.0f, 1.0f },
            { 9000.0f,   9.0f, 0.7f }
        };

        // "Теплее <-> Ярче"

        nodes[0].gainDb += -t * 3.0f;
        nodes[1].gainDb += -t * 1.5f;

        nodes[5].gainDb +=  t * 2.0f;
        nodes[6].gainDb +=  t * 3.0f;

        for (std::size_t i = 0; i < bands.size(); ++i)
        {
            if (i == 0)
            {
                auto coefficients =
                    juce::dsp::IIR::Coefficients<float>::makeHighPass (
                        sampleRate,
                        nodes[i].frequency,
                        nodes[i].q
                    );

                for (auto& filter : bands[i])
                    filter.coefficients = coefficients;
            }
            else
            {
                auto coefficients =
                    juce::dsp::IIR::Coefficients<float>::makePeakFilter (
                        sampleRate,
                        nodes[i].frequency,
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

    float processSample (int channel, float x)
    {
        const std::size_t ch =
            static_cast<std::size_t> (
                juce::jlimit (0, 1, channel)
            );

        for (auto& band : bands)
            x = band[ch].processSample (x);

        return x;
    }

private:

    double sampleRate = 44100.0;

    std::array<
        std::array<
            juce::dsp::IIR::Filter<float>,
            2
        >,
        7
    > bands;
};
