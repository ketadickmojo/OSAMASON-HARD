#include "PluginProcessor.h"
#include "PluginEditor.h"

VocalChainOneEditor::VocalChainOneEditor (VocalChainOneProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p)
{
    // =========================================================
    // KNOBS
    // =========================================================

    toneSlider.setSliderStyle (
        juce::Slider::RotaryHorizontalVerticalDrag);

    toneSlider.setTextBoxStyle (
        juce::Slider::TextBoxBelow,
        false,
        70,
        20);

    toneSlider.setRange (
        -50.0,
        50.0,
        0.1);

    toneSlider.setValue (
        0.0,
        juce::dontSendNotification);

    addAndMakeVisible (toneSlider);

    punchSlider.setSliderStyle (
        juce::Slider::RotaryHorizontalVerticalDrag);

    punchSlider.setTextBoxStyle (
        juce::Slider::TextBoxBelow,
        false,
        70,
        20);

    punchSlider.setRange (
        0.0,
        100.0,
        0.1);

    punchSlider.setValue (
        70.0,
        juce::dontSendNotification);

    addAndMakeVisible (punchSlider);

    loudnessSlider.setSliderStyle (
        juce::Slider::RotaryHorizontalVerticalDrag);

    loudnessSlider.setTextBoxStyle (
        juce::Slider::TextBoxBelow,
        false,
        70,
        20);

    loudnessSlider.setRange (
        0.0,
        100.0,
        0.1);

    // Центр = 50 = 0 dB
    loudnessSlider.setValue (
        50.0,
        juce::dontSendNotification);

    addAndMakeVisible (loudnessSlider);

    gritSlider.setSliderStyle (
        juce::Slider::RotaryHorizontalVerticalDrag);

    gritSlider.setTextBoxStyle (
        juce::Slider::TextBoxBelow,
        false,
        70,
        20);

    gritSlider.setRange (
        0.0,
        100.0,
        0.1);

    gritSlider.setValue (
        40.0,
        juce::dontSendNotification);

    addAndMakeVisible (gritSlider);

    spaceSlider.setSliderStyle (
        juce::Slider::RotaryHorizontalVerticalDrag);

    spaceSlider.setTextBoxStyle (
        juce::Slider::TextBoxBelow,
        false,
        70,
        20);

    spaceSlider.setRange (
        0.0,
        100.0,
        0.1);

    spaceSlider.setValue (
        45.0,
        juce::dontSendNotification);

    addAndMakeVisible (spaceSlider);

    // =========================================================
    // LABELS
    // =========================================================

    toneLabel.setText (
        "TEPLEE -- YARCHE",
        juce::dontSendNotification);

    toneLabel.setJustificationType (
        juce::Justification::centred);

    addAndMakeVisible (toneLabel);

    punchLabel.setText (
        "COMPRESSION",
        juce::dontSendNotification);

    punchLabel.setJustificationType (
        juce::Justification::centred);

    addAndMakeVisible (punchLabel);

    loudnessLabel.setText (
        "LOUDNESS",
        juce::dontSendNotification);

    loudnessLabel.setJustificationType (
        juce::Justification::centred);

    addAndMakeVisible (loudnessLabel);

    gritLabel.setText (
        "GRIT",
        juce::dontSendNotification);

    gritLabel.setJustificationType (
        juce::Justification::centred);

    addAndMakeVisible (gritLabel);

    spaceLabel.setText (
        "SPACE",
        juce::dontSendNotification);

    spaceLabel.setJustificationType (
        juce::Justification::centred);

    addAndMakeVisible (spaceLabel);

    // =========================================================
    // PARAMETER ATTACHMENTS
    // =========================================================

    toneAttach =
        std::make_unique<Attachment> (
            processor.apvts,
            "tone",
            toneSlider);

    punchAttach =
        std::make_unique<Attachment> (
            processor.apvts,
            "punch",
            punchSlider);

    loudnessAttach =
        std::make_unique<Attachment> (
            processor.apvts,
            "loudness",
            loudnessSlider);

    gritAttach =
        std::make_unique<Attachment> (
            processor.apvts,
            "grit",
            gritSlider);

    spaceAttach =
        std::make_unique<Attachment> (
            processor.apvts,
            "space",
            spaceSlider);

    // =========================================================
    // WINDOW
    // =========================================================

    setSize (
        700,
        300);

    setResizable (
        false,
        false);
}

void VocalChainOneEditor::paint (
    juce::Graphics& g)
{
    // =========================================================
    // BACKGROUND
    // =========================================================

    g.fillAll (
        juce::Colour (18, 18, 18));

    // =========================================================
    // TITLE
    // =========================================================

    g.setColour (
        juce::Colours::white);

    g.setFont (
        24.0f);

    g.drawFittedText (
        "VOCAL CHAIN ONE",
        0,
        15,
        getWidth(),
        35,
        juce::Justification::centred,
        1);

    // =========================================================
    // SUBTITLE
    // =========================================================

    g.setColour (
        juce::Colours::lightgrey);

    g.setFont (
        12.0f);

    g.drawFittedText (
        "VOCAL PROCESSING",
        0,
        48,
        getWidth(),
        20,
        juce::Justification::centred,
        1);
}

void VocalChainOneEditor::resized()
{
    // =========================================================
    // LAYOUT
    // =========================================================

    const int knobSize = 100;
    const int spacing = 30;

    const int totalWidth =
        knobSize * 5
        + spacing * 4;

    const int startX =
        (getWidth() - totalWidth) / 2;

    const int knobY = 95;
    const int labelY = 215;

    // =========================================================
    // TONE
    // =========================================================

    toneSlider.setBounds (
        startX,
        knobY,
        knobSize,
        knobSize);

    toneLabel.setBounds (
        startX - 15,
        labelY,
        knobSize + 30,
        25);

    // =========================================================
    // COMPRESSION
    // =========================================================

    const int punchX =
        startX + (knobSize + spacing);

    punchSlider.setBounds (
        punchX,
        knobY,
        knobSize,
        knobSize);

    punchLabel.setBounds (
        punchX - 15,
        labelY,
        knobSize + 30,
        25);

    // =========================================================
    // LOUDNESS
    // =========================================================

    const int loudnessX =
        startX + 2 * (knobSize + spacing);

    loudnessSlider.setBounds (
        loudnessX,
        knobY,
        knobSize,
        knobSize);

    loudnessLabel.setBounds (
        loudnessX - 15,
        labelY,
        knobSize + 30,
        25);

    // =========================================================
    // GRIT
    // =========================================================

    const int gritX =
        startX + 3 * (knobSize + spacing);

    gritSlider.setBounds (
        gritX,
        knobY,
        knobSize,
        knobSize);

    gritLabel.setBounds (
        gritX - 15,
        labelY,
        knobSize + 30,
        25);

    // =========================================================
    // SPACE
    // =========================================================

    const int spaceX =
        startX + 4 * (knobSize + spacing);

    spaceSlider.setBounds (
        spaceX,
        knobY,
        knobSize,
        knobSize);

    spaceLabel.setBounds (
        spaceX - 15,
        labelY,
        knobSize + 30,
        25);
}
