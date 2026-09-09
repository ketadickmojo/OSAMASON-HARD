#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class VocalChainOneEditor : public juce::AudioProcessorEditor
{
public:
    explicit VocalChainOneEditor (
        VocalChainOneProcessor&);

    ~VocalChainOneEditor() override;

    void paint (
        juce::Graphics&) override;

    void resized() override;

private:
    VocalChainOneProcessor& processor;

    // ============================================================
    // MAIN KNOBS
    // ============================================================

    juce::Slider toneSlider;
    juce::Slider punchSlider;
    juce::Slider loudnessSlider;
    juce::Slider gritSlider;
    juce::Slider spaceSlider;

    juce::Label toneLabel;
    juce::Label punchLabel;
    juce::Label loudnessLabel;
    juce::Label gritLabel;
    juce::Label spaceLabel;

    // ============================================================
    // MAIN BUTTONS
    // ============================================================

    juce::TextButton autoTuneButton;
    juce::TextButton resetPresetButton;

    // ============================================================
    // ATTACHMENTS
    // ============================================================

    using Attachment =
        juce::AudioProcessorValueTreeState::SliderAttachment;

    std::unique_ptr<Attachment> toneAttach;
    std::unique_ptr<Attachment> punchAttach;
    std::unique_ptr<Attachment> loudnessAttach;
    std::unique_ptr<Attachment> gritAttach;
    std::unique_ptr<Attachment> spaceAttach;

    // ============================================================
    // AUTO-TUNE WINDOW
    // ============================================================

    std::unique_ptr<juce::DocumentWindow> autoTuneWindow;

    void openAutoTuneWindow();

    // ============================================================
    // RESET MAIN PRESET
    // ============================================================

    void resetMainPreset();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (
        VocalChainOneEditor)
};
