#pragma once
#include <juce_dsp/juce_dsp.h>

/**
    ReverbStage — эмуляция Fruity Reverb 2 поверх juce::dsp::Reverb.
    Wet зафиксирован на 6%, как задал пользователь (это "капля" пространства,
    не выраженный хвост).

    space (0..100, макро "Пространство", общий с Flangus) двигает Size/Decay.
*/
class ReverbStage
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        reverb.prepare (spec);
        update (50.0f);
    }

    void reset() { reverb.reset(); }

    void update (float space)
    {
        float n = juce::jlimit (0.0f, 100.0f, space) / 100.0f;
        auto p = reverb.getParameters();
        p.roomSize   = juce::jmap (n, 0.0f, 1.0f, 0.25f, 0.75f);
        p.damping    = 0.5f;
        p.width      = 1.0f;
        p.freezeMode = 0.0f;
        p.dryLevel   = 1.0f; // сухой/мокрый баланс делаем сами снаружи (fixed wet)
        p.wetLevel   = 1.0f;
        reverb.setParameters (p);
    }

    void process (juce::dsp::AudioBlock<float>& block)
    {
        juce::dsp::ProcessContextReplacing<float> ctx (block);
        reverb.process (ctx);
    }

    static constexpr float kWetFixed = 0.06f; // 6%, как задал пользователь

private:
    juce::dsp::Reverb reverb;
};
