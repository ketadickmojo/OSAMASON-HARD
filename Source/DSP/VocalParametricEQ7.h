#pragma once
#include <juce_dsp/juce_dsp.h>

/**
    VocalParametricEQ7 — реконструкция кривой с 7 узлами, снятой со скриншота
    Fruity Parametric EQ 2 пользователя: плавный подъём от суб-баса, лёгкий провал
    в верхнем миде, широкий подъём к области presence/treble.

    Точных значений в дБ на скрине не было (только визуальная кривая), поэтому
    цифры ниже — восстановленная по форме кривой аппроксимация. tone от общей
    крутилки "Теплее -- Ярче" сдвигает баланс между низкими узлами (C1-C3) и
    верхними (C6-C7).
*/
class VocalParametricEQ7
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        for (auto* f : bands) f->prepare (spec);
        update (0.0f);
    }

    void reset() { for (auto* f : bands) f->reset(); }

    void update (float tone)
    {
        float t = juce::jlimit (-1.0f, 1.0f, tone / 50.0f);

        // Базовые узлы (freq, gainDb, Q) -- восстановлены по форме кривой скрина
        struct Node { float freq, gainDb, q; };
        Node nodes[7] = {
            { 30.0f,   -18.0f, 0.6f },  // C1 -- глубокий сабовый спад (как высокий срез)
            { 100.0f,    2.0f, 0.9f },  // C2 -- бас
            { 250.0f,    1.0f, 1.0f },  // C3 -- низкая середина
            { 800.0f,    6.0f, 1.1f },  // C4 -- апекс кривой (презенс низкого мида)
            { 2000.0f,  -2.0f, 1.2f },  // C5 -- лёгкий провал (убираем резкость)
            { 5000.0f,   6.0f, 1.0f },  // C6 -- верхний мид/presence
            { 9000.0f,   9.0f, 0.7f },  // C7 -- воздух/треш-шельф
        };

        // tone > 0 (ярче): усиливаем C6/C7, слегка приглушаем C1/C2
        // tone < 0 (теплее): наоборот
        nodes[0].gainDb += -t * 3.0f;
        nodes[1].gainDb += -t * 1.5f;
        nodes[5].gainDb +=  t * 2.0f;
        nodes[6].gainDb +=  t * 3.0f;

        for (int i = 0; i < 7; ++i)
        {
            if (i == 0)
                *bands[i]->state = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, nodes[i].freq, nodes[i].q);
            else
                *bands[i]->state = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate, nodes[i].freq, nodes[i].q,
                                        juce::Decibels::decibelsToGain (nodes[i].gainDb));
        }
    }

    float processSample (int channel, float x)
    {
        for (auto* f : bands)
            x = f->processSample (channel, x);
        return x;
    }

private:
    double sampleRate = 44100.0;
    using Filter = juce::dsp::IIR::Filter<float>;
    Filter band1, band2, band3, band4, band5, band6, band7;
    Filter* bands[7] = { &band1, &band2, &band3, &band4, &band5, &band6, &band7 };
};
