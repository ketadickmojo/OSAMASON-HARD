#pragma once
#include <juce_dsp/juce_dsp.h>

/**
    CorrectionEQ — первый блок цепи (Slot 1 на скрине пользователя).
    Базовые значения зафиксированы: 90 Hz -11dB (low shelf), 1.5 kHz +3.3dB (bell),
    8 kHz +9.5dB (high shelf).

    tone (-50..+50) слегка наклоняет весь EQ: отрицательный tone усиливает бас
    и приглушает верх (теплее), положительный — наоборот (ярче).
*/
class CorrectionEQ
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        for (auto* f : { &lowShelf, &bell, &highShelf })
            f->prepare (spec);
        update (0.0f);
    }

    void reset()
    {
        for (auto* f : { &lowShelf, &bell, &highShelf })
            f->reset();
    }

    // tone: -50..+50, из общей крутилки "Теплее -- Ярче"
    void update (float tone)
    {
        float toneNorm = juce::jlimit (-1.0f, 1.0f, tone / 50.0f);

        // При "теплее" (tone<0) — усиливаем низкий шельф и ослабляем верхний,
        // при "ярче" (tone>0) — наоборот. Средняя полоса чуть подстраивается для баланса.
        float lowGainDb  = -11.0f + toneNorm * -4.0f;   // теплее -> менее отрицательно (больше баса)
        float bellGainDb =   3.3f + toneNorm *  1.0f;
        float highGainDb =   9.5f + toneNorm *  4.0f;   // ярче -> ещё выше

        *lowShelf.state  = *juce::dsp::IIR::Coefficients<float>::makeLowShelf  (sampleRate, 90.0f,  0.71f, juce::Decibels::decibelsToGain (lowGainDb));
        *bell.state      = *juce::dsp::IIR::Coefficients<float>::makePeakFilter (sampleRate, 1500.0f, 1.0f,  juce::Decibels::decibelsToGain (bellGainDb));
        *highShelf.state = *juce::dsp::IIR::Coefficients<float>::makeHighShelf (sampleRate, 8000.0f, 0.71f, juce::Decibels::decibelsToGain (highGainDb));
    }

    float processSample (int channel, float x)
    {
        x = lowShelf.processSample (channel, x);
        x = bell.processSample (channel, x);
        x = highShelf.processSample (channel, x);
        return x;
    }

private:
    double sampleRate = 44100.0;
    juce::dsp::IIR::Filter<float> lowShelf, bell, highShelf;
};
