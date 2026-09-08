#include "RealtimePitchDetector.h"

void RealtimePitchDetector::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;

    if (sampleRate <= 0.0)
        sampleRate = 44100.0;

    reset();
}

void RealtimePitchDetector::reset()
{
    buffer.fill (0.0f);

    writeIndex = 0;
    samplesSinceAnalysis = 0;

    smoothedPitch = 0.0f;
    confidence = 0.0f;
}

float RealtimePitchDetector::process (float sample)
{
    buffer[static_cast<std::size_t> (writeIndex)] = sample;

    ++writeIndex;

    if (writeIndex >= bufferSize)
        writeIndex = 0;

    ++samplesSinceAnalysis;

    if (samplesSinceAnalysis >= analysisHop)
    {
        samplesSinceAnalysis = 0;

        const float detectedPitch = detectPitch();

        if (detectedPitch > 0.0f)
        {
            if (smoothedPitch <= 0.0f)
            {
                smoothedPitch = detectedPitch;
            }
            else
            {
                // Light smoothing prevents rapid octave/jitter movement.
                smoothedPitch =
                    smoothedPitch * 0.75f
                    + detectedPitch * 0.25f;
            }
        }
        else
        {
            // Slowly decay the last pitch instead of dropping instantly.
            smoothedPitch *= 0.92f;

            if (smoothedPitch < minFrequency)
                smoothedPitch = 0.0f;
        }
    }

    return smoothedPitch;
}

float RealtimePitchDetector::correlationAt (int lag) const
{
    if (lag <= 0 || lag >= bufferSize)
        return 0.0f;

    // Keep the analysis window short enough for realtime use,
    // while still covering several periods of a male/female vocal.
    constexpr int analysisSamples = 768;

    double sumXY = 0.0;
    double sumXX = 0.0;
    double sumYY = 0.0;

    for (int i = 0; i < analysisSamples; ++i)
    {
        const int indexX =
            (writeIndex + i) % bufferSize;

        const int indexY =
            (writeIndex + i + lag) % bufferSize;

        const float x =
            buffer[static_cast<std::size_t> (indexX)];

        const float y =
            buffer[static_cast<std::size_t> (indexY)];

        sumXY += static_cast<double> (x) * y;
        sumXX += static_cast<double> (x) * x;
        sumYY += static_cast<double> (y) * y;
    }

    const double denominator =
        std::sqrt (sumXX * sumYY);

    if (denominator < 1.0e-12)
        return 0.0f;

    return static_cast<float> (sumXY / denominator);
}

float RealtimePitchDetector::interpolatedLag (
    int lag,
    float ym1,
    float y,
    float yp1) const
{
    const float denominator =
        ym1
        - 2.0f * y
        + yp1;

    if (std::abs (denominator) < 1.0e-9f)
        return static_cast<float> (lag);

    const float delta =
        0.5f * (ym1 - yp1) / denominator;

    return static_cast<float> (lag) + delta;
}

float RealtimePitchDetector::detectPitch()
{
    int localMinLag =
        static_cast<int> (
            sampleRate / maxFrequency);

    int localMaxLag =
        static_cast<int> (
            sampleRate / minFrequency);

    // Clamp the lag range to the detector's buffer.
    if (localMinLag < 1)
        localMinLag = 1;

    if (localMaxLag > bufferSize - 2)
        localMaxLag = bufferSize - 2;

    if (localMinLag >= localMaxLag)
        return 0.0f;

    // ---------------------------------------------------------
    // RMS / SILENCE CHECK
    // ---------------------------------------------------------

    double energy = 0.0;

    for (int i = 0; i < bufferSize; ++i)
    {
        const float x =
            buffer[
                static_cast<std::size_t> (
                    (writeIndex + i) % bufferSize)];

        energy +=
            static_cast<double> (x) * x;
    }

    energy /=
        static_cast<double> (bufferSize);

    if (energy < 1.0e-7)
    {
        confidence = 0.0f;
        return 0.0f;
    }

    const float rms =
        std::sqrt (static_cast<float> (energy));

    if (rms < 0.001f)
    {
        confidence = 0.0f;
        return 0.0f;
    }

    // ---------------------------------------------------------
    // AUTOCORRELATION SEARCH
    // ---------------------------------------------------------

    float bestCorrelation = 0.0f;
    int bestLag = -1;

    for (int lag = localMinLag;
         lag <= localMaxLag;
         ++lag)
    {
        const float correlation =
            correlationAt (lag);

        if (correlation > bestCorrelation)
        {
            bestCorrelation = correlation;
            bestLag = lag;
        }
    }

    // ---------------------------------------------------------
    // CONFIDENCE CHECK
    // ---------------------------------------------------------

    if (bestLag < 0
        || bestCorrelation < minimumCorrelation)
    {
        confidence = 0.0f;
        return 0.0f;
    }

    confidence = bestCorrelation;

    // ---------------------------------------------------------
    // SUB-SAMPLE LAG INTERPOLATION
    // ---------------------------------------------------------

    float lag =
        static_cast<float> (bestLag);

    if (bestLag > localMinLag
        && bestLag < localMaxLag)
    {
        const float ym1 =
            correlationAt (bestLag - 1);

        const float y =
            correlationAt (bestLag);

        const float yp1 =
            correlationAt (bestLag + 1);

        lag =
            interpolatedLag (
                bestLag,
                ym1,
                y,
                yp1);
    }

    if (lag <= 0.0f)
        return 0.0f;

    // ---------------------------------------------------------
    // LAG -> FREQUENCY
    // ---------------------------------------------------------

    const float frequency =
        static_cast<float> (sampleRate) / lag;

    if (frequency < minFrequency
        || frequency > maxFrequency)
    {
        return 0.0f;
    }

    return frequency;
}
