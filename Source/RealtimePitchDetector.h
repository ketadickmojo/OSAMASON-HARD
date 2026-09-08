#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

class RealtimePitchDetector
{
public:
    void prepare (
        double newSampleRate);

    void reset();

    float process (
        float input);

private:
    double sampleRate = 44100.0;

    float lastPitch = 0.0f;

    float previousSample = 0.0f;
    float previousDifference = 0.0f;

    float periodSamples = 0.0f;
    float confidence = 0.0f;
};
