#pragma once
#include <juce_dsp/juce_dsp.h>
#include <cmath>

/**
    FreshAirStage — эмуляция Fresh Air (Slate Digital): два "воздушных" эксайтера --
    Low (презенс, ~3-6kHz) и High (эйр, ~10-16kHz), оба гармонически насыщают
    высокочастотный контент и подмешивают обратно как высокочастотный шельф.

    Пользователь задал оба параметра на +8 (из 10) -- берём как базу.
    tone (-50..+50, общая крутилка "Теплее -- Ярче") слегка сдвигает баланс:
    ярче -> оба чуть сильнее, теплее -> оба чуть слабее.
*/
class FreshAirStage
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        lowBandFilter.prepare (spec);
        highBandFilter.prepare (spec);
        *lowBandFilter.state  = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, 3500.0f, 0.7f);
        *highBandFilter.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass (sampleRate, 10000.0f, 0.7f);
        update (0.0f);
    }

    void reset() { lowBandFilter.reset(); highBandFilter.reset(); }

    void update (float tone)
    {
        float t = juce::jlimit (-1.0f, 1.0f, tone / 50.0f);
        // база +8 из 10 = 0.8, диапазон эффекта модулируем tone на +-0.15
        lowAmount  = juce::jlimit (0.0f, 1.0f, 0.8f + t * 0.15f);
        highAmount = juce::jlimit (0.0f, 1.0f, 0.8f + t * 0.15f);
    }

    float processSample (int channel, float x)
    {
        float lowBand  = lowBandFilter.processSample (channel, x);
        float highBand = highBandFilter.processSample (channel, x);

        // Гармоническая генерация (чётные+нечётные через asymmetric soft-clip) на каждой полосе
        float lowHarm  = exciter (lowBand)  * lowAmount  * 0.6f;
        float highHarm = exciter (highBand) * highAmount * 0.6f;

        return x + lowHarm + highHarm;
    }

private:
    double sampleRate = 44100.0;
    float lowAmount = 0.8f, highAmount = 0.8f;
    using juce::dsp::IIR::Filter<float>;
    Filter lowBandFilter, highBandFilter;

    static float exciter (float x)
    {
        // Ассиметричная сатурация -- добавляет чётные гармоники (воздух/блеск)
        return std::tanh (x * 3.0f) - 0.3f * std::tanh (x * x * 3.0f) * (x < 0 ? -1.0f : 1.0f);
    }
};
