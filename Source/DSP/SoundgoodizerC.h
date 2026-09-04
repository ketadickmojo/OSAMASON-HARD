#pragma once
#include <cmath>
#include <algorithm>

/**
    SoundgoodizerC — упрощённая эмуляция Soundgoodizer в режиме C ("Loud"):
    сочетание мягкого ограничения уровня и гармонического насыщения,
    даёт ощущение "плотности" без выраженной классической компрессии.

    amount (0..100) из макро "Громкость" -- на скрине круть была почти на максимум,
    поэтому по умолчанию будем стартовать высоко, но крутилка даёт полный диапазон.
*/
class SoundgoodizerC
{
public:
    void prepare (double sampleRate) { fs = sampleRate; (void) fs; }
    void reset() { envelope = 0.0f; }

    void update (float amount) { amt = std::clamp (amount, 0.0f, 100.0f) / 100.0f; }

    float processSample (float x)
    {
        // Огибающая для мягкого "дыхания" эффекта (быстрая атака/умеренный релиз)
        float target = std::abs (x);
        envelope += (target - envelope) * (target > envelope ? 0.5f : 0.05f);

        // Мягкое насыщение, сила зависит от amount и текущей огибающей (программно-зависимое)
        float drive = 1.0f + amt * 3.0f * (0.5f + 0.5f * envelope);
        float saturated = std::tanh (x * drive) / std::tanh (drive);

        // Небольшая компенсация громкости, чтобы "громче" ощущалось честно, а не только гуще
        float makeup = 1.0f + amt * 0.3f;

        return x * (1.0f - amt) + saturated * makeup * amt;
    }

private:
    double fs = 44100.0;
    float amt = 0.8f;
    float envelope = 0.0f;
};
