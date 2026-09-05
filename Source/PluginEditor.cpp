#include "PluginProcessor.h"
#include "PluginEditor.h"

// =============================================================
// VOCAL CHAIN ONE
// RAGE / INDUSTRIAL LOOK
// =============================================================

namespace
{
    class RageLookAndFeel : public juce::LookAndFeel_V4
    {
    public:

        RageLookAndFeel()
        {
            setColour (
                juce::Slider::thumbColourId,
                juce::Colour (190, 55, 255));

            setColour (
                juce::Slider::rotarySliderFillColourId,
                juce::Colour (170, 35, 255));

            setColour (
                juce::Slider::rotarySliderOutlineColourId,
                juce::Colour (55, 20, 65));

            setColour (
                juce::Slider::textBoxTextColourId,
                juce::Colour (220, 180, 255));

            setColour (
                juce::Slider::textBoxBackgroundColourId,
                juce::Colour (12, 8, 15));

            setColour (
                juce::Slider::textBoxOutlineColourId,
                juce::Colour (75, 30, 90));

            setColour (
                juce::Label::textColourId,
                juce::Colour (190, 65, 255));
        }

        void drawRotarySlider (
            juce::Graphics& g,
            int x,
            int y,
            int width,
            int height,
            float sliderPosProportional,
            float rotaryStartAngle,
            float rotaryEndAngle,
            juce::Slider& slider) override
        {
            const float cx =
                x + width * 0.5f;

            const float cy =
                y + height * 0.5f;

            const float radius =
                juce::jmin (width, height) * 0.36f;

            // -------------------------------------------------
            // OUTER DARK RING
            // -------------------------------------------------

            g.setColour (
                juce::Colour (8, 6, 10));

            g.fillEllipse (
                cx - radius - 13.0f,
                cy - radius - 13.0f,
                (radius + 13.0f) * 2.0f,
                (radius + 13.0f) * 2.0f);

            // -------------------------------------------------
            // METAL RING
            // -------------------------------------------------

            g.setColour (
                juce::Colour (45, 35, 50));

            g.drawEllipse (
                cx - radius - 10.0f,
                cy - radius - 10.0f,
                (radius + 10.0f) * 2.0f,
                (radius + 10.0f) * 2.0f,
                3.0f);

            // -------------------------------------------------
            // PURPLE VALUE ARC
            // -------------------------------------------------

            juce::Path valueArc;

            valueArc.addCentredArc (
                cx,
                cy,
                radius + 7.0f,
                radius + 7.0f,
                0.0f,
                rotaryStartAngle,
                rotaryStartAngle
                    + sliderPosProportional
                    * (rotaryEndAngle - rotaryStartAngle),
                true);

            g.setColour (
                juce::Colour (175, 30, 255));

            g.strokePath (
                valueArc,
                juce::PathStrokeType (
                    5.0f,
                    juce::PathStrokeType::curved,
                    juce::PathStrokeType::rounded));

            // -------------------------------------------------
            // DARK KNOB BODY
            // -------------------------------------------------

            g.setColour (
                juce::Colour (14, 11, 17));

            g.fillEllipse (
                cx - radius,
                cy - radius,
                radius * 2.0f,
                radius * 2.0f);

            // -------------------------------------------------
            // KNOB INNER RING
            // -------------------------------------------------

            g.setColour (
                juce::Colour (70, 30, 85));

            g.drawEllipse (
                cx - radius + 4.0f,
                cy - radius + 4.0f,
                (radius - 4.0f) * 2.0f,
                (radius - 4.0f) * 2.0f,
                2.0f);

            // -------------------------------------------------
            // KNOB HIGHLIGHT
            // -------------------------------------------------

            g.setColour (
                juce::Colour (115, 45, 145));

            g.drawEllipse (
                cx - radius + 8.0f,
                cy - radius + 8.0f,
                (radius - 8.0f) * 2.0f,
                (radius - 8.0f) * 2.0f,
                1.0f);

            // -------------------------------------------------
            // POSITION INDICATOR
            // -------------------------------------------------

            const float angle =
                rotaryStartAngle
                + sliderPosProportional
                * (rotaryEndAngle - rotaryStartAngle);

            const float indicatorLength =
                radius * 0.70f;

            const float indicatorX =
                cx + std::cos (angle)
                * indicatorLength;

            const float indicatorY =
                cy + std::sin (angle)
                * indicatorLength;

            g.setColour (
                juce::Colour (220, 80, 255));

            g.drawLine (
                cx,
                cy,
                indicatorX,
                indicatorY,
                3.0f);

            // -------------------------------------------------
            // CENTER
            // -------------------------------------------------

            g.setColour (
                juce::Colour (175, 35, 255));

            g.fillEllipse (
                cx - 3.0f,
                cy - 3.0f,
                6.0f,
                6.0f);
        }

        void drawLabel (
            juce::Graphics& g,
            juce::Label& label) override
        {
            g.setColour (
                juce::Colour (190, 55, 255));

            g.setFont (
                juce::Font (
                    13.0f,
                    juce::Font::bold));

            g.drawFittedText (
                label.getText(),
                label.getLocalBounds(),
                juce::Justification::centred,
                1);
        }
    };
}


// =============================================================
// CONSTRUCTOR
// =============================================================

VocalChainOneEditor::VocalChainOneEditor (
    VocalChainOneProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p)
{
    // ---------------------------------------------------------
    // LOOK & FEEL
    // ---------------------------------------------------------

    static RageLookAndFeel rageLookAndFeel;

    // ---------------------------------------------------------
    // TONE
    // ---------------------------------------------------------

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

    toneSlider.setLookAndFeel (
        &rageLookAndFeel);

    addAndMakeVisible (
        toneSlider);

    // ---------------------------------------------------------
    // COMPRESSION
    // ---------------------------------------------------------

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

    punchSlider.setLookAndFeel (
        &rageLookAndFeel);

    addAndMakeVisible (
        punchSlider);

    // ---------------------------------------------------------
    // LOUDNESS
    // ---------------------------------------------------------

    loudnessSlider.setSliderStyle (
        juce::Slider::RotaryHorizontalVerticalDrag);

    loudnessSlider.setTextBoxStyle (
        juce::Slider::TextBoxBelow,
        false,
        80,
        20);

    loudnessSlider.setRange (
        0.0,
        100.0,
        0.1);

    loudnessSlider.setValue (
        50.0,
        juce::dontSendNotification);

    loudnessSlider.setTextValueSuffix (
        " %");

    loudnessSlider.setLookAndFeel (
        &rageLookAndFeel);

    addAndMakeVisible (
        loudnessSlider);

    // ---------------------------------------------------------
    // GRIT
    // ---------------------------------------------------------

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

    gritSlider.setLookAndFeel (
        &rageLookAndFeel);

    addAndMakeVisible (
        gritSlider);

    // ---------------------------------------------------------
    // SPACE
    // ---------------------------------------------------------

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

    spaceSlider.setLookAndFeel (
        &rageLookAndFeel);

    addAndMakeVisible (
        spaceSlider);

    // =========================================================
    // LABELS
    // =========================================================

    toneLabel.setText (
        "TEPLEE -- YARCHE",
        juce::dontSendNotification);

    toneLabel.setJustificationType (
        juce::Justification::centred);

    toneLabel.setLookAndFeel (
        &rageLookAndFeel);

    addAndMakeVisible (
        toneLabel);

    punchLabel.setText (
        "COMPRESSION",
        juce::dontSendNotification);

    punchLabel.setJustificationType (
        juce::Justification::centred);

    punchLabel.setLookAndFeel (
        &rageLookAndFeel);

    addAndMakeVisible (
        punchLabel);

    loudnessLabel.setText (
        "LOUDNESS",
        juce::dontSendNotification);

    loudnessLabel.setJustificationType (
        juce::Justification::centred);

    loudnessLabel.setLookAndFeel (
        &rageLookAndFeel);

    addAndMakeVisible (
        loudnessLabel);

    gritLabel.setText (
        "GRIT",
        juce::dontSendNotification);

    gritLabel.setJustificationType (
        juce::Justification::centred);

    gritLabel.setLookAndFeel (
        &rageLookAndFeel);

    addAndMakeVisible (
        gritLabel);

    spaceLabel.setText (
        "SPACE",
        juce::dontSendNotification);

    spaceLabel.setJustificationType (
        juce::Justification::centred);

    spaceLabel.setLookAndFeel (
        &rageLookAndFeel);

    addAndMakeVisible (
        spaceLabel);

    // =========================================================
    // ATTACHMENTS
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
        900,
        430);

    setResizable (
        false,
        false);
}


// =============================================================
// PAINT
// =============================================================

void VocalChainOneEditor::paint (
    juce::Graphics& g)
{
    const auto bounds =
        getLocalBounds().toFloat();

    // =========================================================
    // BACKGROUND
    // =========================================================

    g.fillAll (
        juce::Colour (5, 4, 7));

    // =========================================================
    // INNER PANEL
    // =========================================================

    g.setColour (
        juce::Colour (10, 8, 13));

    g.fillRoundedRectangle (
        bounds.reduced (8.0f),
        6.0f);

    // =========================================================
    // PURPLE BORDER
    // =========================================================

    g.setColour (
        juce::Colour (95, 25, 125));

    g.drawRoundedRectangle (
        bounds.reduced (8.0f),
        6.0f,
        2.0f);

    // =========================================================
    // TOP GLITCH LINES
    // =========================================================

    g.setColour (
        juce::Colour (135, 35, 180));

    for (int i = 0; i < 8; ++i)
    {
        const float y =
            68.0f + i * 3.0f;

        const float x =
            20.0f + (i % 3) * 40.0f;

        g.drawLine (
            x,
            y,
            getWidth() - 20.0f - i * 35.0f,
            y,
            1.0f);
    }

    // =========================================================
    // TITLE
    // =========================================================

    g.setColour (
        juce::Colour (215, 70, 255));

    g.setFont (
        juce::Font (
            34.0f,
            juce::Font::bold));

    g.drawFittedText (
        "VOCAL CHAIN ONE",
        150,
        18,
        600,
        45,
        juce::Justification::centred,
        1);

    // =========================================================
    // SUBTITLE
    // =========================================================

    g.setColour (
        juce::Colour (125, 90, 140));

    g.setFont (
        juce::Font (
            11.0f,
            juce::Font::bold));

    g.drawFittedText (
        "PSYCHOTIC VOCAL PROCESSOR // VC1",
        150,
        58,
        600,
        20,
        juce::Justification::centred,
        1);

    // =========================================================
    // LEFT VC1 BLOCK
    // =========================================================

    g.setColour (
        juce::Colour (25, 18, 30));

    g.fillRect (
        22,
        18,
        100,
        60);

    g.setColour (
        juce::Colour (120, 30, 155));

    g.drawRect (
        22,
        18,
        100,
        60,
        2);

    g.setColour (
        juce::Colour (205, 65, 255));

    g.setFont (
        juce::Font (
            23.0f,
            juce::Font::bold));

    g.drawFittedText (
        "VC1",
        22,
        25,
        100,
        28,
        juce::Justification::centred,
        1);

    g.setFont (
        9.0f);

    g.drawFittedText (
        "// RAGE UNIT",
        22,
        52,
        100,
        15,
        juce::Justification::centred,
        1);

    // =========================================================
    // ACTIVE INDICATOR
    // =========================================================

    g.setColour (
        juce::Colour (25, 18, 30));

    g.fillRoundedRectangle (
        getWidth() - 130.0f,
        22.0f,
        105.0f,
        35.0f,
        5.0f);

    g.setColour (
        juce::Colour (160, 35, 210));

    g.drawRoundedRectangle (
        getWidth() - 130.0f,
        22.0f,
        105.0f,
        35.0f,
        5.0f,
        1.5f);

    g.setColour (
        juce::Colour (205, 55, 255));

    g.fillEllipse (
        getWidth() - 115.0f,
        34.0f,
        10.0f,
        10.0f);

    g.setFont (
        juce::Font (
            12.0f,
            juce::Font::bold));

    g.drawText (
        "ACTIVE",
        getWidth() - 98,
        30,
        60,
        20,
        juce::Justification::centredLeft);

    // =========================================================
    // MAIN PANEL
    // =========================================================

    g.setColour (
        juce::Colour (18, 12, 22));

    g.fillRoundedRectangle (
        20.0f,
        95.0f,
        getWidth() - 40.0f,
        290.0f,
        8.0f);

    g.setColour (
        juce::Colour (75, 25, 90));

    g.drawRoundedRectangle (
        20.0f,
        95.0f,
        getWidth() - 40.0f,
        290.0f,
        8.0f,
        2.0f);

    // =========================================================
    // DIVIDER LINES
    // =========================================================

    g.setColour (
        juce::Colour (45, 20, 55));

    for (int i = 1; i < 5; ++i)
    {
        const float x =
            20.0f
            + i * ((getWidth() - 40.0f) / 5.0f);

        g.drawLine (
            x,
            110.0f,
            x,
            370.0f,
            1.0f);
    }

    // =========================================================
    // GRUNGE / SCRATCHES
    // =========================================================

    g.setColour (
        juce::Colour (70, 25, 85));

    for (int i = 0; i < 24; ++i)
    {
        const int x =
            30 + ((i * 137) % (getWidth() - 60));

        const int y =
            105 + ((i * 71) % 260);

        const int length =
            8 + ((i * 17) % 35);

        g.drawLine (
            (float) x,
            (float) y,
            (float) (x + length),
            (float) (y - 2),
            1.0f);
    }

    // =========================================================
    // BOTTOM TEXT
    // =========================================================

    g.setColour (
        juce::Colour (90, 50, 105));

    g.setFont (
        juce::Font (
            10.0f,
            juce::Font::bold));

    g.drawFittedText (
        "/// HARD VOCAL PROCESSING /// DO NOT TRUST THE SIGNAL ///",
        40,
        395,
        getWidth() - 80,
        20,
        juce::Justification::centred,
        1);

    // =========================================================
    // CORNER MARKS
    // =========================================================

    g.setColour (
        juce::Colour (150, 40, 190));

    const float s = 12.0f;

    // top left
    g.drawLine (12, 12, 12 + s, 12, 2.0f);
    g.drawLine (12, 12, 12, 12 + s, 2.0f);

    // top right
    g.drawLine (getWidth() - 12, 12,
                getWidth() - 12 - s, 12, 2.0f);

    g.drawLine (getWidth() - 12, 12,
                getWidth() - 12, 12 + s, 2.0f);

    // bottom left
    g.drawLine (12, getHeight() - 12,
                12 + s, getHeight() - 12, 2.0f);

    g.drawLine (12, getHeight() - 12,
                12, getHeight() - 12 - s, 2.0f);

    // bottom right
    g.drawLine (getWidth() - 12, getHeight() - 12,
                getWidth() - 12 - s, getHeight() - 12, 2.0f);

    g.drawLine (getWidth() - 12, getHeight() - 12,
                getWidth() - 12,
                getHeight() - 12 - s,
                2.0f);
}


// =============================================================
// RESIZED
// =============================================================

void VocalChainOneEditor::resized()
{
    const int knobSize = 115;

    const int sectionWidth =
        (getWidth() - 40) / 5;

    const int knobY = 145;

    const int labelY = 270;

    // ---------------------------------------------------------
    // TONE
    // ---------------------------------------------------------

    toneSlider.setBounds (
        20,
        knobY,
        sectionWidth,
        knobSize);

    toneLabel.setBounds (
        25,
        labelY,
        sectionWidth - 10,
        25);

    // ---------------------------------------------------------
    // COMPRESSION
    // ---------------------------------------------------------

    punchSlider.setBounds (
        20 + sectionWidth,
        knobY,
        sectionWidth,
        knobSize);

    punchLabel.setBounds (
        25 + sectionWidth,
        labelY,
        sectionWidth - 10,
        25);

    // ---------------------------------------------------------
    // LOUDNESS
    // ---------------------------------------------------------

    loudnessSlider.setBounds (
        20 + sectionWidth * 2,
        knobY,
        sectionWidth,
        knobSize);

    loudnessLabel.setBounds (
        25 + sectionWidth * 2,
        labelY,
        sectionWidth - 10,
        25);

    // ---------------------------------------------------------
    // GRIT
    // ---------------------------------------------------------

    gritSlider.setBounds (
        20 + sectionWidth * 3,
        knobY,
        sectionWidth,
        knobSize);

    gritLabel.setBounds (
        25 + sectionWidth * 3,
        labelY,
        sectionWidth - 10,
        25);

    // ---------------------------------------------------------
    // SPACE
    // ---------------------------------------------------------

    spaceSlider.setBounds (
        20 + sectionWidth * 4,
        knobY,
        sectionWidth,
        knobSize);

    spaceLabel.setBounds (
        25 + sectionWidth * 4,
        labelY,
        sectionWidth - 10,
        25);
}
