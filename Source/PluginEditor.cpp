#include "PluginEditor.h"

namespace
{
    void setupKnob (juce::Slider& s, juce::Label& l, juce::Component& parent, const juce::String& text)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
        parent.addAndMakeVisible (s);

        l.setText (text, juce::dontSendNotification);
        l.setJustificationType (juce::Justification::centred);
        l.setFont (juce::Font (15.0f, juce::Font::bold));
        parent.addAndMakeVisible (l);
    }
}

VocalChainOneEditor::VocalChainOneEditor (VocalChainOneProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setupKnob (toneSlider,     toneLabel,     *this, "Теплее -- Ярче");
    setupKnob (punchSlider,    punchLabel,    *this, "Сжатие вокала");
    setupKnob (loudnessSlider, loudnessLabel, *this, "Громкость");
    setupKnob (gritSlider,     gritLabel,     *this, "Грязь");
    setupKnob (spaceSlider,    spaceLabel,    *this, "Пространство");

    auto& apvts = processor.apvts;
    toneAttach     = std::make_unique<Attachment> (apvts, "tone",     toneSlider);
    punchAttach    = std::make_unique<Attachment> (apvts, "punch",    punchSlider);
    loudnessAttach = std::make_unique<Attachment> (apvts, "loudness", loudnessSlider);
    gritAttach     = std::make_unique<Attachment> (apvts, "grit",     gritSlider);
    spaceAttach    = std::make_unique<Attachment> (apvts, "space",    spaceSlider);

    setSize (620, 260);
}

void VocalChainOneEditor::paint (juce::Graphics& g)
{
    // Временный placeholder-фон -- дизайн дорабатываем отдельно по твоему вкусу
    g.fillAll (juce::Colour (0xff1c1c22));

    g.setColour (juce::Colour (0xffe8d9b5));
    g.setFont (juce::Font (22.0f, juce::Font::bold));
    g.drawText ("VOCAL CHAIN ONE", getLocalBounds().removeFromTop (36),
                juce::Justification::centred);
}

void VocalChainOneEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    area.removeFromTop (36); // заголовок

    auto knobWidth = area.getWidth() / 5;

    auto place = [&] (juce::Slider& s, juce::Label& l)
    {
        auto col = area.removeFromLeft (knobWidth);
        s.setBounds (col.reduced (6).removeFromTop (col.getHeight() - 30));
        l.setBounds (col.removeFromBottom (30));
    };

    place (toneSlider, toneLabel);
    place (punchSlider, punchLabel);
    place (loudnessSlider, loudnessLabel);
    place (gritSlider, gritLabel);
    place (spaceSlider, spaceLabel);
}
