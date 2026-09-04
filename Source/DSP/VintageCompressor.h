#pragma once
#include <cmath>
#include <algorithm>

/**
    VintageCompressor — реализует точные цифры со скрина Fruity Compressor:
    Threshold -7.4dB, Ratio 8:1, Gain (makeup) +15dB, Attack 60.7ms, Release 200ms,
    тип Vintage (мягкое колено + лёгкая гармоническая сатурация).

    Attack/Release/Gain — фиксированы (это "характер" звука пользователя, крутилкой
    не трогаем). Threshold и Ratio едут от макро "Сжатие вокала" (punch, 0..100):
    punch=100 воспроизводит точные цифры пользователя (-7.4dB, 8:1),
    punch=0 — гораздо мягче (-3dB, 2:1), чтобы диапазон был осмысленным в обе стороны.
*/
class VintageCompressor
{
public:
    void prepare (double sampleRate)
    {
        fs = sampleRate;
        attackCoeff  = calcCoeff (attackMs);
        releaseCoeff = calcCoeff (releaseMs);
    }

    void reset() { envelopeDb = -60.0f; }

    // punch: 0..100, из макро-крутилки "Сжатие вокала"
    void update (float punch)
    {
        float n = std::clamp (punch, 0.0f, 100.0f) / 100.0f;
        thresholdDb = -3.0f  + n * (-7.4f - -3.0f);   // -3 .. -7.4
        ratio       =  2.0f  + n * (8.0f  - 2.0f);    //  2 .. 8
    }

    float processSample (float x)
    {
        float levelDb = linToDb (std::abs (x) + 1.0e-9f);

        float coeff = (levelDb > envelopeDb) ? attackCoeff : releaseCoeff;
        envelopeDb += (levelDb - envelopeDb) * coeff;

        float overshoot = envelopeDb - thresholdDb;
        float grDb = 0.0f;
        if (overshoot > 0.0f)
            grDb = overshoot * (1.0f - 1.0f / ratio);

        float grLin = dbToLin (-grDb);
        float compressed = x * grLin * dbToLin (makeupDb);

        // "Vintage" характер -- лёгкая насыщенность, пропорциональная глубине сжатия
        float sat = std::clamp (grDb / 15.0f, 0.0f, 1.0f) * 0.25f;
        if (sat > 0.001f)
            compressed = compressed * (1.0f - sat) + std::tanh (compressed * 1.5f) * sat;

        return compressed;
    }

private:
    double fs = 44100.0;
    float thresholdDb = -7.4f, ratio = 8.0f;
    const float makeupDb = 15.0f;     // фиксировано, как на скрине
    const float attackMs = 60.7f;     // фиксировано
    const float releaseMs = 200.0f;   // фиксировано

    float envelopeDb = -60.0f;
    float attackCoeff = 0.01f, releaseCoeff = 0.001f;

    float calcCoeff (float ms) const { return 1.0f - std::exp (-1.0f / (0.001f * ms * (float) fs)); }
    static float dbToLin (float db) { return std::pow (10.0f, db / 20.0f); }
    static float linToDb (float lin) { return 20.0f * std::log10 (lin); }
};
