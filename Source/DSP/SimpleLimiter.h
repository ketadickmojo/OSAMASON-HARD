#pragma once
#include <cmath>
#include <algorithm>

class SimpleLimiter
{
public:
    void prepare (double sr)
    {
        sampleRate = sr;
        reset();
        update();
    }

    void reset()
    {
        envelope = 0.0f;
        gain = 1.0f;
    }

    // Fruity Limiter:
    // Gain: 0 dB
    // Soft Saturation Threshold: 0
    // Ceiling: 0 dB
    // Attack: 2.00 ms
    // Attack Curve: 3
    // Release: 85.53 ms
    // Release Curve: 3
    // Noise Release: 226 ms
    // Noise Gate: 0
    // Noise Threshold: -43 dB
    //
    // Важно:
    // громкость пользователя сюда НЕ приходит.
    // Лимитер всегда работает с фиксированными настройками.

    void update()
    {
        ceilingLinear = 1.0f; // 0 dB

        attackTime = 0.002f;      // 2 ms
        releaseTime = 0.08553f;   // 85.53 ms

        attackCoeff =
            std::exp (-1.0f / (float (sampleRate) * attackTime));

        releaseCoeff =
            std::exp (-1.0f / (float (sampleRate) * releaseTime));
    }

    float processSample (float x)
    {
        const float inputAbs = std::abs (x);

        // Peak envelope
        if (inputAbs > envelope)
            envelope = attackCoeff * envelope
                      + (1.0f - attackCoeff) * inputAbs;
        else
            envelope = releaseCoeff * envelope
                      + (1.0f - releaseCoeff) * inputAbs;

        float targetGain = 1.0f;

        if (envelope > ceilingLinear)
            targetGain = ceilingLinear / envelope;

        // Smooth gain changes
        if (targetGain < gain)
        {
            gain = attackCoeff * gain
                 + (1.0f - attackCoeff) * targetGain;
        }
        else
        {
            gain = releaseCoeff * gain
                 + (1.0f - releaseCoeff) * targetGain;
        }

        return x * gain;
    }

private:
    double sampleRate = 44100.0;

    float envelope = 0.0f;
    float gain = 1.0f;

    float ceilingLinear = 1.0f;

    float attackTime = 0.002f;
    float releaseTime = 0.08553f;

    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
};
