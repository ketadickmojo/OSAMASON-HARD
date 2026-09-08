#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>

#include "RealtimePitchDetector.h"

#include <array>
#include <cmath>
#include <limits>

class AutoTuneEngine
{
public:
    enum class ScaleType
    {
        Major,
        Minor,
        Chromatic
    };

    void prepare(
        double newSampleRate,
        int newSamplesPerBlock);

    void reset();

    void setEnabled(bool shouldBeEnabled);
    void setKey(int newKey);
    void setScale(ScaleType newScale);
    void setRetuneSpeed(float newSpeed);
    void setAmount(float newAmount);

    void process(
        juce::AudioBuffer<float>& buffer);

private:
    static constexpr int delaySize = 8192;

    float processSample(
        float input,
        bool rightChannel);

    float frequencyToMidi(
        float frequency) const;

    float midiToFrequency(
        float midi) const;

    bool isNoteAllowed(
        int midiNote) const;

    float getTargetMidiNote(
        float detectedMidi) const;

    float semitoneDistance(
        float from,
        float to) const;

    float getRetuneCoefficient() const;

    float readDelay(
        const std::array<float, delaySize>& delay,
        float position) const;

    RealtimePitchDetector detector;

    double sampleRate = 44100.0;
    int blockSize = 128;

    bool enabled = false;

    int key = 0;

    ScaleType scale = ScaleType::Major;

    float retuneSpeed = 50.0f;
    float amount = 100.0f;

    std::array<float, delaySize> delayL {};
    std::array<float, delaySize> delayR {};

    int delayWritePosition = 0;

    float currentPitch = 0.0f;
    float targetPitch = 0.0f;

    float correctionSemitones = 0.0f;
    float smoothedCorrection = 0.0f;

    float oscillatorPhase = 0.0f;
};
