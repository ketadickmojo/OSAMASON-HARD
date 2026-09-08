#pragma once

#include <JuceHeader.h>
#include <array>

class RealtimePitchDetector
{
public:
    void prepare (double sampleRate);
    void reset();

    float process (float sample);

private:
    double sampleRate = 48000.0;

    static constexpr int bufferSize = 1024;
    static constexpr int minLag = 32;
    static constexpr int maxLag = 512;

    std::array<float, bufferSize> buffer {};
    int writeIndex = 0;
    int samplesSinceAnalysis = 0;

    float smoothedPitch = 0.0f;
    float confidence = 0.0f;

    float detectPitch() const;

    float interpolatedLag (
        int lag,
        float ym1,
        float y,
        float yp1) const;
};
