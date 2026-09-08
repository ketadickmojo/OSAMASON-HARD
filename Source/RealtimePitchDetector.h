#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include <array>
#include <cmath>

class RealtimePitchDetector
{
public:
    void prepare(double newSampleRate);
    void reset();

    float process(float sample);

private:
    static constexpr int bufferSize = 2048;
    static constexpr int analysisHop = 64;

    float detectPitch();

    double sampleRate = 44100.0;

    std::array<float, bufferSize> buffer {};

    int writeIndex = 0;
    int samplesSinceAnalysis = 0;

    float smoothedPitch = 0.0f;

    float lastPitch = 0.0f;
    float previousSample = 0.0f;
    float previousDifference = 0.0f;

    float periodSamples = 0.0f;
    float confidence = 0.0f;
};
