#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class VocalChainOneEditor : public juce::AudioProcessorEditor
{
public:
    explicit VocalChainOneEditor (VocalChainOneProcessor&);
    ~VocalChainOneEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VocalChainOneProcessor& processor;

    juce::Slider toneSlider, punchSlider, loudnessSlider, gritSlider, spaceSlider;
    juce::Label  toneLabel, punchLabel, loudnessLabel, gritLabel, spaceLabel;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<Attachment> toneAttach, punchAttach, loudnessAttach, gritAttach, spaceAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalChainOneEditor)
};
