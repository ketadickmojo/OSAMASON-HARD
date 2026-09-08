#pragma once

#include <array>
#include <cmath>

class RealtimePitchDetector
{
public:
    void prepare (double newSampleRate);
    void reset();

    // Feeds one mono sample and returns the currently estimated pitch in Hz.
    // Returns 0.0f when no reliable pitch is detected.
    float process (float sample);

private:
    // 2048 samples gives enough period information for the lower
    // part of the vocal range while keeping CPU reasonable.
    static constexpr int bufferSize = 2048;

    // Re-run pitch analysis every 64 samples.
    static constexpr int analysisHop = 64;

    // Vocal range used by this detector.
    static constexpr float minFrequency = 80.0f;
    static constexpr float maxFrequency = 1000.0f;

    // Minimum correlation required to accept a pitch estimate.
    static constexpr float minimumCorrelation = 0.60f;

    float detectPitch();

    float interpolatedLag (
        int lag,
        float ym1,
        float y,
        float yp1) const;

    float correlationAt (int lag) const;

    double sampleRate = 44100.0;

    std::array<float, bufferSize> buffer {};

    int writeIndex = 0;
    int samplesSinceAnalysis = 0;

    float smoothedPitch = 0.0f;
    float confidence = 0.0f;
};
