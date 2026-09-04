#pragma once
#include <juce_dsp/juce_dsp.h>

/**
    FlangusStage — эмуляция Fruity Flangus поверх juce::dsp::Chorus (модулированная
    задержка с обратной связью). Wet зафиксирован на 17%, как задал пользователь.

    space (0..100, макро "Пространство") двигает Depth и Speed совместно с реверб-модулем.
*/
class FlangusStage
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        chorus.prepare (spec);
        chorus.setCentreDelay (7.0f);   // мс, база под флэнжер-диапазон
        chorus.setFeedback (0.25f);
        chorus.setMix (1.0f);           // мешаем сухой/мокрый сами снаружи (fixed wet)
        update (50.0f);
    }

    void reset() { chorus.reset(); }

    void update (float space)
    {
        float n = juce::jlimit (0.0f, 100.0f, space) / 100.0f;
        chorus.setDepth (juce::jmap (n, 0.0f, 1.0f, 0.15f, 0.55f));
        chorus.setRate  (juce::jmap (n, 0.0f, 1.0f, 0.15f, 0.6f));
    }

    void process (juce::dsp::AudioBlock<float>& block)
    {
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        chorus.process (ctx);
    }

    static constexpr float kWetFixed = 0.17f; // 17%, как задал пользователь

private:
    juce::dsp::Chorus<float> chorus;
};
