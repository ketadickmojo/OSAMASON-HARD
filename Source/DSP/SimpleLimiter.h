#pragma once
#include <cmath>
#include <algorithm>

/**
    SimpleLimiter — пиковый лимитер с плавным release, эмулирует Fruity Limiter
    в режиме LIMIT. Ceiling и makeup gain связаны с макро-крутилкой Loudness:
    чем больше Loudness, тем сильнее лимитер "поджимает" пики и тем выше makeup.
*/
class SimpleLimiter
{
public:
    void prepare (double sampleRate) { fs = sampleRate; releaseCoeff = calcCoeff (releaseMs); }

    void reset() { envelope = 1.0f; }

    // loudness: 0..100, из макро-крутилки "Громкость"
    void update (float loudness)
    {
        float n = jlimit_local (0.0f, 100.0f, loudness) / 100.0f;
        ceilingDb = jmap_local (n, 0.0f, 1.0f, -0.3f, -3.0f);  // больше Loudness -> ниже потолок (жёстче лимит)
        makeupDb  = jmap_local (n, 0.0f, 1.0f, 0.0f, 6.0f);    // и больше компенсации громкости
    }

    float processSample (float x)
    {
        float driven = x * dbToLin (makeupDb);
        float ceilingLin = dbToLin (ceilingDb);

        float absX = std::abs (driven);
        float targetGain = (absX > ceilingLin && absX > 1.0e-9f) ? (ceilingLin / absX) : 1.0f;

        // Быстрая атака (лимитер должен ловить пики почти мгновенно), плавный release
        if (targetGain < envelope)
            envelope = targetGain; // мгновенная атака
        else
            envelope += (targetGain - envelope) * releaseCoeff;

        return driven * envelope;
    }

private:
    double fs = 44100.0;
    float envelope = 1.0f;
    float ceilingDb = -0.3f, makeupDb = 0.0f;
    float releaseMs = 80.0f;
    float releaseCoeff = 0.001f;

    float calcCoeff (float ms) const
    {
        return 1.0f - std::exp (-1.0f / (0.001f * ms * (float) fs));
    }

    static float dbToLin (float db) { return std::pow (10.0f, db / 20.0f); }

    // локальные хелперы, чтобы не тянуть juce_core только ради jlimit/jmap
    template <typename T> static T jlimit_local (T lo, T hi, T v) { return std::max (lo, std::min (hi, v)); }
    template <typename T> static T jmap_local (T v, T s1, T e1, T s2, T e2) { return s2 + (v - s1) * (e2 - s2) / (e1 - s1); }
};
