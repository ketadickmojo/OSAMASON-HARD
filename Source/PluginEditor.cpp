#include "PluginEditor.h"

namespace
{
    // ============================================================
    // MAIN LOOK & FEEL
    // ============================================================

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

            setColour (
                juce::TextButton::buttonColourId,
                juce::Colour (18, 10, 24));

            setColour (
                juce::TextButton::buttonOnColourId,
                juce::Colour (95, 20, 125));

            setColour (
                juce::TextButton::textColourOffId,
                juce::Colour (210, 90, 255));

            setColour (
                juce::TextButton::textColourOnId,
                juce::Colour (240, 160, 255));
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
            juce::Slider&) override
        {
            const float cx =
                x + width * 0.5f;

            const float cy =
                y + height * 0.5f;

            const float radius =
                juce::jmin (
                    width,
                    height)
                * 0.36f;

            // Outer ring
            g.setColour (
                juce::Colour (8, 6, 10));

            g.fillEllipse (
                cx - radius - 13.0f,
                cy - radius - 13.0f,
                (radius + 13.0f) * 2.0f,
                (radius + 13.0f) * 2.0f);

            // Metal ring
            g.setColour (
                juce::Colour (45, 35, 50));

            g.drawEllipse (
                cx - radius - 10.0f,
                cy - radius - 10.0f,
                (radius + 10.0f) * 2.0f,
                (radius + 10.0f) * 2.0f,
                3.0f);

            // Value arc
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
                    * (rotaryEndAngle
                       - rotaryStartAngle),
                true);

            g.setColour (
                juce::Colour (175, 30, 255));

            g.strokePath (
                valueArc,
                juce::PathStrokeType (
                    5.0f,
                    juce::PathStrokeType::curved,
                    juce::PathStrokeType::rounded));

            // Knob body
            g.setColour (
                juce::Colour (14, 11, 17));

            g.fillEllipse (
                cx - radius,
                cy - radius,
                radius * 2.0f,
                radius * 2.0f);

            // Inner ring
            g.setColour (
                juce::Colour (70, 30, 85));

            g.drawEllipse (
                cx - radius + 4.0f,
                cy - radius + 4.0f,
                (radius - 4.0f) * 2.0f,
                (radius - 4.0f) * 2.0f,
                2.0f);

            // Highlight
            g.setColour (
                juce::Colour (115, 45, 145));

            g.drawEllipse (
                cx - radius + 8.0f,
                cy - radius + 8.0f,
                (radius - 8.0f) * 2.0f,
                (radius - 8.0f) * 2.0f,
                1.0f);

            // Indicator
            const float angle =
                rotaryStartAngle
                + sliderPosProportional
                * (rotaryEndAngle
                   - rotaryStartAngle);

            const float indicatorLength =
                radius * 0.70f;

            const float indicatorX =
                cx
                + std::cos (angle)
                * indicatorLength;

            const float indicatorY =
                cy
                + std::sin (angle)
                * indicatorLength;

            g.setColour (
                juce::Colour (220, 80, 255));

            g.drawLine (
                cx,
                cy,
                indicatorX,
                indicatorY,
                3.0f);

            // Center
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

    // ============================================================
    // AUTO-TUNE WINDOW CONTENT
    // ============================================================

    class AutoTuneContent :
        public juce::Component
    {
    public:

        explicit AutoTuneContent (
            VocalChainOneProcessor& p)
            : processor (p)
        {
            static RageLookAndFeel rageLookAndFeel;

            // ----------------------------------------------------
            // ENABLE
            // ----------------------------------------------------

            enabledButton.setButtonText (
                "AUTO-TUNE OFF");

            enabledButton.setClickingTogglesState (
                true);

            enabledButton.setLookAndFeel (
                &rageLookAndFeel);

            addAndMakeVisible (
                enabledButton);

            enabledAttachment =
                std::make_unique<
                    juce::AudioProcessorValueTreeState::ButtonAttachment>(
                        processor.apvts,
                        "autotuneEnabled",
                        enabledButton);

            // ----------------------------------------------------
            // KEY
            // ----------------------------------------------------

            keyLabel.setText (
                "KEY",
                juce::dontSendNotification);

            keyLabel.setJustificationType (
                juce::Justification::centred);

            keyLabel.setLookAndFeel (
                &rageLookAndFeel);

            addAndMakeVisible (
                keyLabel);

            keyBox.addItem ("C", 1);
            keyBox.addItem ("C#", 2);
            keyBox.addItem ("D", 3);
            keyBox.addItem ("D#", 4);
            keyBox.addItem ("E", 5);
            keyBox.addItem ("F", 6);
            keyBox.addItem ("F#", 7);
            keyBox.addItem ("G", 8);
            keyBox.addItem ("G#", 9);
            keyBox.addItem ("A", 10);
            keyBox.addItem ("A#", 11);
            keyBox.addItem ("B", 12);

            keyBox.setLookAndFeel (
                &rageLookAndFeel);

            addAndMakeVisible (
                keyBox);

            keyAttachment =
                std::make_unique<
                    juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                        processor.apvts,
                        "autotuneKey",
                        keyBox);

            // ----------------------------------------------------
            // SCALE
            // ----------------------------------------------------

            scaleLabel.setText (
                "SCALE",
                juce::dontSendNotification);

            scaleLabel.setJustificationType (
                juce::Justification::centred);

            scaleLabel.setLookAndFeel (
                &rageLookAndFeel);

            addAndMakeVisible (
                scaleLabel);

            scaleBox.addItem (
                "MAJOR",
                1);

            scaleBox.addItem (
                "MINOR",
                2);

            scaleBox.addItem (
                "CHROMATIC",
                3);

            scaleBox.setLookAndFeel (
                &rageLookAndFeel);

            addAndMakeVisible (
                scaleBox);

            scaleAttachment =
                std::make_unique<
                    juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
                        processor.apvts,
                        "autotuneScale",
                        scaleBox);

            // ----------------------------------------------------
            // RETUNE SPEED
            // ----------------------------------------------------

            setupKnob (
                retuneSlider,
                retuneLabel,
                "RETUNE SPEED",
                0.0,
                100.0,
                50.0);

            retuneAttachment =
                std::make_unique<
                    juce::AudioProcessorValueTreeState::SliderAttachment>(
                        processor.apvts,
                        "autotuneRetune",
                        retuneSlider);

            // ----------------------------------------------------
            // AMOUNT
            // ----------------------------------------------------

            setupKnob (
                amountSlider,
                amountLabel,
                "AMOUNT",
                0.0,
                100.0,
                100.0);

            amountAttachment =
                std::make_unique<
                    juce::AudioProcessorValueTreeState::SliderAttachment>(
                        processor.apvts,
                        "autotuneAmount",
                        amountSlider);

            setSize (
                520,
                390);
        }

        ~AutoTuneContent() override
        {
            keyBox.setLookAndFeel (nullptr);
            scaleBox.setLookAndFeel (nullptr);
            enabledButton.setLookAndFeel (nullptr);
            retuneSlider.setLookAndFeel (nullptr);
            amountSlider.setLookAndFeel (nullptr);
            retuneLabel.setLookAndFeel (nullptr);
            amountLabel.setLookAndFeel (nullptr);
            keyLabel.setLookAndFeel (nullptr);
            scaleLabel.setLookAndFeel (nullptr);
        }

        void paint (
            juce::Graphics& g) override
        {
            g.fillAll (
                juce::Colour (5, 3, 8));

            const auto bounds =
                getLocalBounds()
                    .toFloat()
                    .reduced (8.0f);

            g.setColour (
                juce::Colour (13, 8, 18));

            g.fillRoundedRectangle (
                bounds,
                8.0f);

            g.setColour (
                juce::Colour (115, 25, 150));

            g.drawRoundedRectangle (
                bounds,
                8.0f,
                2.0f);

            g.setColour (
                juce::Colour (220, 70, 255));

            g.setFont (
                juce::Font (
                    30.0f,
                    juce::Font::bold));

            g.drawFittedText (
                "AUTO-TUNE",
                20,
                18,
                getWidth() - 40,
                42,
                juce::Justification::centred,
                1);

            g.setColour (
                juce::Colour (105, 65, 125));

            g.setFont (
                juce::Font (
                    10.0f,
                    juce::Font::bold));

            g.drawFittedText (
                "REALTIME PITCH CORRECTION // VC1",
                20,
                58,
                getWidth() - 40,
                18,
                juce::Justification::centred,
                1);
        }

        void resized() override
        {
            enabledButton.setBounds (
                145,
                90,
                230,
                42);

            keyLabel.setBounds (
                75,
                145,
                150,
                22);

            scaleLabel.setBounds (
                295,
                145,
                150,
                22);

            keyBox.setBounds (
                75,
                170,
                150,
                34);

            scaleBox.setBounds (
                295,
                170,
                150,
                34);

            retuneSlider.setBounds (
                70,
                220,
                170,
                130);

            amountSlider.setBounds (
                280,
                220,
                170,
                130);
        }

    private:

        VocalChainOneProcessor& processor;

        juce::ToggleButton enabledButton;

        juce::ComboBox keyBox;
        juce::ComboBox scaleBox;

        juce::Label keyLabel;
        juce::Label scaleLabel;

        juce::Slider retuneSlider;
        juce::Slider amountSlider;

        juce::Label retuneLabel;
        juce::Label amountLabel;

        using SliderAttachment =
            juce::AudioProcessorValueTreeState::SliderAttachment;

        using ComboAttachment =
            juce::AudioProcessorValueTreeState::ComboBoxAttachment;

        using ButtonAttachment =
            juce::AudioProcessorValueTreeState::ButtonAttachment;

        std::unique_ptr<SliderAttachment> retuneAttachment;
        std::unique_ptr<SliderAttachment> amountAttachment;

        std::unique_ptr<ComboAttachment> keyAttachment;
        std::unique_ptr<ComboAttachment> scaleAttachment;

        std::unique_ptr<ButtonAttachment> enabledAttachment;

        void setupKnob (
            juce::Slider& slider,
            juce::Label& label,
            const juce::String& text,
            double min,
            double max,
            double value)
        {
            static RageLookAndFeel rageLookAndFeel;

            slider.setSliderStyle (
                juce::Slider::RotaryHorizontalVerticalDrag);

            slider.setTextBoxStyle (
                juce::Slider::TextBoxBelow,
                false,
                75,
                20);

            slider.setRange (
                min,
                max,
                0.1);

            slider.setValue (
                value,
                juce::dontSendNotification);

            slider.setLookAndFeel (
                &rageLookAndFeel);

            addAndMakeVisible (
                slider);

            label.setText (
                text,
                juce::dontSendNotification);

            label.setJustificationType (
                juce::Justification::centred);

            label.setLookAndFeel (
                &rageLookAndFeel);

            addAndMakeVisible (
                label);
        }

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (
            AutoTuneContent)
    };

    // ============================================================
    // AUTO-TUNE WINDOW
    // ============================================================

    class AutoTuneWindow :
        public juce::DocumentWindow
    {
    public:

        AutoTuneWindow (
            VocalChainOneProcessor& processor)
            : DocumentWindow (
                "AUTO-TUNE",
                juce::Colour (8, 5, 11),
                DocumentWindow::closeButton)
        {
            setUsingNativeTitleBar (true);

            setContentOwned (
                new AutoTuneContent (
                    processor),
                true);

            centreWithSize (
                520,
                390);

            setResizable (
                false,
                false);
        }

        void closeButtonPressed() override
        {
            setVisible (
                false);
        }
    };
}

// ================================================================
// CONSTRUCTOR
// ================================================================

VocalChainOneEditor::VocalChainOneEditor (
    VocalChainOneProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p)
{
    static RageLookAndFeel rageLookAndFeel;

    // ============================================================
    // TONE
    // ============================================================

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

    // ============================================================
    // COMPRESSION
    // ============================================================

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

    // ============================================================
    // LOUDNESS
    // ============================================================

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

    // ============================================================
    // GRIT
    // ============================================================

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

    // ============================================================
    // SPACE
    // ============================================================

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

    // ============================================================
    // LABELS
    // ============================================================

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

    // ============================================================
    // AUTO-TUNE BUTTON
    // ============================================================

    autoTuneButton.setButtonText (
        "AUTO-TUNE");

    autoTuneButton.setLookAndFeel (
        &rageLookAndFeel);

    autoTuneButton.onClick =
        [this]
        {
            openAutoTuneWindow();
        };

    addAndMakeVisible (
        autoTuneButton);

    // ============================================================
    // RESET PRESET BUTTON
    // ============================================================

    resetPresetButton.setButtonText (
        "RESET PRESET");

    resetPresetButton.setLookAndFeel (
        &rageLookAndFeel);

    resetPresetButton.onClick =
        [this]
        {
            resetMainPreset();
        };

    addAndMakeVisible (
        resetPresetButton);

    // ============================================================
    // ATTACHMENTS
    // ============================================================

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

    // ============================================================
    // WINDOW
    // ============================================================

    setSize (
        900,
        430);

    setResizable (
        false,
        false);
}

// ================================================================
// DESTRUCTOR
// ================================================================

VocalChainOneEditor::~VocalChainOneEditor()
{
    if (autoTuneWindow != nullptr)
    {
        autoTuneWindow->setVisible (
            false);

        autoTuneWindow.reset();
    }

    toneSlider.setLookAndFeel (nullptr);
    punchSlider.setLookAndFeel (nullptr);
    loudnessSlider.setLookAndFeel (nullptr);
    gritSlider.setLookAndFeel (nullptr);
    spaceSlider.setLookAndFeel (nullptr);

    toneLabel.setLookAndFeel (nullptr);
    punchLabel.setLookAndFeel (nullptr);
    loudnessLabel.setLookAndFeel (nullptr);
    gritLabel.setLookAndFeel (nullptr);
    spaceLabel.setLookAndFeel (nullptr);

    autoTuneButton.setLookAndFeel (nullptr);
    resetPresetButton.setLookAndFeel (nullptr);
}

// ================================================================
// OPEN AUTO-TUNE WINDOW
// ================================================================

void VocalChainOneEditor::openAutoTuneWindow()
{
    if (autoTuneWindow == nullptr)
    {
        autoTuneWindow =
            std::make_unique<AutoTuneWindow> (
                processor);
    }

    autoTuneWindow->setVisible (
        true);

    autoTuneWindow->toFront (
        true);
}

// ================================================================
// RESET MAIN PRESET
// ================================================================

void VocalChainOneEditor::resetMainPreset()
{
    // IMPORTANT:
    // This resets ONLY the original VocalChainOne controls.
    //
    // Auto-Tune parameters are deliberately NOT touched.

    processor.apvts.getParameter (
        "tone")
        ->setValueNotifyingHost (
            processor.apvts
                .getParameterRange ("tone")
                .convertTo0to1 (
                    0.0f));

    processor.apvts.getParameter (
        "punch")
        ->setValueNotifyingHost (
            processor.apvts
                .getParameterRange ("punch")
                .convertTo0to1 (
                    70.0f));

    processor.apvts.getParameter (
        "loudness")
        ->setValueNotifyingHost (
            processor.apvts
                .getParameterRange ("loudness")
                .convertTo0to1 (
                    50.0f));

    processor.apvts.getParameter (
        "grit")
        ->setValueNotifyingHost (
            processor.apvts
                .getParameterRange ("grit")
                .convertTo0to1 (
                    40.0f));

    processor.apvts.getParameter (
        "space")
        ->setValueNotifyingHost (
            processor.apvts
                .getParameterRange ("space")
                .convertTo0to1 (
                    45.0f));
}

// ================================================================
// PAINT
// ================================================================

void VocalChainOneEditor::paint (
    juce::Graphics& g)
{
    const auto bounds =
        getLocalBounds()
            .toFloat();

    // ============================================================
    // BACKGROUND
    // ============================================================

    g.fillAll (
        juce::Colour (5, 4, 7));

    // ============================================================
    // INNER PANEL
    // ============================================================

    g.setColour (
        juce::Colour (10, 8, 13));

    g.fillRoundedRectangle (
        bounds.reduced (8.0f),
        6.0f);

    // ============================================================
    // PURPLE BORDER
    // ============================================================

    g.setColour (
        juce::Colour (95, 25, 125));

    g.drawRoundedRectangle (
        bounds.reduced (8.0f),
        6.0f,
        2.0f);

    // ============================================================
    // TOP GLITCH LINES
    // ============================================================

    g.setColour (
        juce::Colour (135, 35, 180));

    for (int i = 0;
         i < 8;
         ++i)
    {
        const float y =
            68.0f
            + i * 3.0f;

        const float x =
            20.0f
            + (i % 3) * 40.0f;

        g.drawLine (
            x,
            y,
            getWidth()
                - 20.0f
                - i * 35.0f,
            y,
            1.0f);
    }

    // ============================================================
    // TITLE
    // ============================================================

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

    // ============================================================
    // SUBTITLE
    // ============================================================

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

    // ============================================================
    // LEFT VC1 BLOCK
    // ============================================================

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

    // ============================================================
    // ACTIVE INDICATOR
    // ============================================================

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

    // ============================================================
    // MAIN PANEL
    // ============================================================

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

    // ============================================================
    // DIVIDER LINES
    // ============================================================

    g.setColour (
        juce::Colour (45, 20, 55));

    for (int i = 1;
         i < 5;
         ++i)
    {
        const float x =
            20.0f
            + i
                * ((getWidth() - 40.0f)
                   / 5.0f);

        g.drawLine (
            x,
            110.0f,
            x,
            370.0f,
            1.0f);
    }

    // ============================================================
    // GRUNGE / SCRATCHES
    // ============================================================

    g.setColour (
        juce::Colour (70, 25, 85));

    for (int i = 0;
         i < 24;
         ++i)
    {
        const int x =
            30
            + ((i * 137)
               % (getWidth() - 60));

        const int y =
            105
            + ((i * 71)
               % 260);

        const int length =
            8
            + ((i * 17)
               % 35);

        g.drawLine (
            static_cast<float> (x),
            static_cast<float> (y),
            static_cast<float> (
                x + length),
            static_cast<float> (
                y - 2),
            1.0f);
    }

    // ============================================================
    // BOTTOM TEXT
    // ============================================================

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

    // ============================================================
    // CORNER MARKS
    // ============================================================

    g.setColour (
        juce::Colour (150, 40, 190));

    const float s = 12.0f;

    // Top left
    g.drawLine (
        12,
        12,
        12 + s,
        12,
        2.0f);

    g.drawLine (
        12,
        12,
        12,
        12 + s,
        2.0f);

    // Top right
    g.drawLine (
        getWidth() - 12,
        12,
        getWidth() - 12 - s,
        12,
        2.0f);

    g.drawLine (
        getWidth() - 12,
        12,
        getWidth() - 12,
        12 + s,
        2.0f);

    // Bottom left
    g.drawLine (
        12,
        getHeight() - 12,
        12 + s,
        getHeight() - 12,
        2.0f);

    g.drawLine (
        12,
        getHeight() - 12,
        12,
        getHeight() - 12 - s,
        2.0f);

    // Bottom right
    g.drawLine (
        getWidth() - 12,
        getHeight() - 12,
        getWidth() - 12 - s,
        getHeight() - 12,
        2.0f);

    g.drawLine (
        getWidth() - 12,
        getHeight() - 12,
        getWidth() - 12,
        getHeight() - 12 - s,
        2.0f);
}

// ================================================================
// RESIZED
// ================================================================

void VocalChainOneEditor::resized()
{
    const int knobSize = 115;

    const int sectionWidth =
        (getWidth() - 40) / 5;

    const int knobY = 145;
    const int labelY = 270;

    // ============================================================
    // TONE
    // ============================================================

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

    // ============================================================
    // COMPRESSION
    // ============================================================

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

    // ============================================================
    // LOUDNESS
    // ============================================================

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

    // ============================================================
    // GRIT
    // ============================================================

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

    // ============================================================
    // SPACE
    // ============================================================

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

    // ============================================================
    // AUTO-TUNE BUTTON
    // ============================================================

    autoTuneButton.setBounds (
        300,
        350,
        150,
        32);

    // ============================================================
    // RESET BUTTON
    // ============================================================

    resetPresetButton.setBounds (
        465,
        350,
        150,
        32);
}
