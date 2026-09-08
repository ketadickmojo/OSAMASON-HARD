#include "RealtimePitchDetector.h"

void RealtimePitchDetector::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;
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
    buffer[writeIndex] = sample;

    writeIndex++;

    if (writeIndex >= bufferSize)
        writeIndex = 0;

    samplesSinceAnalysis++;

    if (samplesSinceAnalysis >= 64)
    {
        samplesSinceAnalysis = 0;

        const float detected = detectPitch();

        if (detected > 0.0f)
        {
            if (smoothedPitch <= 0.0f)
                smoothedPitch = detected;
            else
                smoothedPitch =
                    smoothedPitch * 0.75f
                    + detected * 0.25f;
        }
        else
        {
            smoothedPitch *= 0.92f;

            if (smoothedPitch < 20.0f)
                smoothedPitch = 0.0f;
        }
    }

    return smoothedPitch;
}

float RealtimePitchDetector::interpolatedLag (
    int lag,
    float ym1,
    float y,
    float yp1) const
{
    const float denominator =
        ym1 - 2.0f * y + yp1;

    if (std::abs (denominator) < 1.0e-9f)
        return static_cast<float> (lag);

    const float delta =
        0.5f * (ym1 - yp1) / denominator;

    return static_cast<float> (lag) + delta;
}

float RealtimePitchDetector::detectPitch() const
{
    constexpr float minFrequency = 80.0f;
    constexpr float maxFrequency = 1000.0f;

    int localMinLag =
        static_cast<int> (
            sampleRate / maxFrequency);

    int localMaxLag =
        static_cast<int> (
            sampleRate / minFrequency);

    localMinLag =
        juce::jlimit (
            minLag,
            maxLag,
            localMinLag);

    localMaxLag =
        juce::jlimit (
            minLag + 1,
            maxLag,
            localMaxLag);

    float bestCorrelation = 0.0f;
    int bestLag = -1;

    double energy = 0.0;

    for (int i = 0; i < bufferSize; ++i)
    {
        const float x =
            buffer[
                (writeIndex + i) % bufferSize
            ];

        energy +=
            static_cast<double> (x) * x;
    }

    energy /=
        static_cast<double> (bufferSize);

    if (energy < 1.0e-7)
        return 0.0f;

    const float rms =
        std::sqrt (
            static_cast<float> (energy));

    if (rms < 0.001f)
        return 0.0f;

    for (int lag = localMinLag;
         lag <= localMaxLag;
         ++lag)
    {
        double sumXY = 0.0;
        double sumXX = 0.0;
        double sumYY = 0.0;

        constexpr int analysisSamples = 768;

        for (int i = 0;
             i < analysisSamples;
             ++i)
        {
            const float x =
                buffer[
                    (writeIndex + i) %
                    bufferSize
                ];

            const float y =
                buffer[
                    (writeIndex + i + lag) %
                    bufferSize
                ];

            sumXY +=
                static_cast<double> (x) * y;

            sumXX +=
                static_cast<double> (x) * x;

            sumYY +=
                static_cast<double> (y) * y;
        }

        const double denominator =
            std::sqrt (sumXX * sumYY);

        if (denominator < 1.0e-12)
            continue;

        const float correlation =
            static_cast<float> (
                sumXY / denominator);

        if (correlation > bestCorrelation)
        {
            bestCorrelation = correlation;
            bestLag = lag;
        }
    }

    if (bestLag < 0 ||
        bestCorrelation < 0.60f)
    {
        return 0.0f;
    }

    confidence = bestCorrelation;

    float lag =
        static_cast<float> (bestLag);

    if (bestLag > localMinLag &&
        bestLag < localMaxLag)
    {
        const auto correlationAt =
            [this] (int lagValue) -> float
            {
                double sumXY = 0.0;
                double sumXX = 0.0;
                double sumYY = 0.0;

                constexpr int samples = 768;

                for (int i = 0;
                     i < samples;
                     ++i)
                {
                    const float x =
                        buffer[
                            (writeIndex + i) %
                            bufferSize
                        ];

                    const float y =
                        buffer[
                            (writeIndex + i + lagValue) %
                            bufferSize
                        ];

                    sumXY +=
                        static_cast<double> (x) * y;

                    sumXX +=
                        static_cast<double> (x) * x;

                    sumYY +=
                        static_cast<double> (y) * y;
                }

                const double denominator =
                    std::sqrt (sumXX * sumYY);

                if (denominator < 1.0e-12)
                    return 0.0f;

                return static_cast<float> (
                    sumXY / denominator);
            };

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

    const float frequency =
        static_cast<float> (sampleRate) / lag;

    if (frequency < minFrequency ||
        frequency > maxFrequency)
    {
        return 0.0f;
    }

    return frequency;
}
