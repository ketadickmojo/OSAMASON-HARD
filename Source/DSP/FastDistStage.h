#pragma once
#include <cmath>
#include <algorithm>

/**
    FastDistStage — эмуляция Fruity Fast Dist в режиме A (жёсткий клиппинг-стиль).
    Wet зафиксирован на 20% (пользователь указал точный процент) -- не выводим
    наружу, чтобы баланс с сухим сигналом не сломали случайно.

    grit (0..100) из макро "Грязь" двигает Thresh (порог клиппинга) и Post (выходной
    уровень внутри цепи дисторшна).
*/
class FastDistStage
{
public:
    void prepare (double sampleRate) { (void) sampleRate; }
    void reset() {}

    void update (float grit)
    {
        float n = std::clamp (grit, 0.0f, 100.0f) / 100.0f;
        threshLin = 1.0f - n * 0.85f;   // больше grit -> ниже порог -> жёстче клип
        postGain  = 1.0f + n * 0.6f;    // компенсация громкости в цепи дисторшна
    }

    float processSample (float x)
    {
        float pre = x * 1.2f; // фиксированный PRE со скрина (небольшой драйв перед порогом)

        // Жёсткий клип по порогу (режим A -- симметричный hard-clip)
        float clipped = std::clamp (pre, -threshLin, threshLin) * postGain;

        return x * (1.0f - kWetFixed) + clipped * kWetFixed;
    }

private:
    static constexpr float kWetFixed = 0.20f; // 20%, как задал пользователь
    float threshLin = 0.5f;
    float postGain = 1.2f;
};
