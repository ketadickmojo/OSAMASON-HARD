#pragma once

#include <JuceHeader.h>
#include "RealtimePitchDetector.h"

class AutoTuneEngine
{
public:
    enum class ScaleType
    {
        Major = 0,
        Minor,
        Chromatic
    };

    void prepare (
        double sampleRate,
        int samplesPerBlock);

    void reset();

    void setEnabled (
        bool shouldBeEnabled);

    void setKey (
        int newKey);

    void setScale (
        ScaleType newScale);

    void setRetuneSpeed (
        float newSpeed);

    void setAmount (
        float newAmount);

    void process (
        juce::AudioBuffer<float>& buffer);

private:
    double sampleRate = 48000.0;
    int blockSize = 128;

    bool enabled = false;

    int key = 0;

    ScaleType scale =
        ScaleType::Major;

    float retuneSpeed = 50.0f;
    float amount = 100.0f;

    RealtimePitchDetector detector;

    float currentPitch = 0.0f;
    float targetPitch = 0.0f;

    float correctionSemitones = 0.0f;
    float smoothedCorrection = 0.0f;

    static constexpr int delaySize = 8192;

    std::array<float, delaySize> delayL {};
    std::array<float, delaySize> delayR {};

    int delayWritePosition = 0;

    float oscillatorPhase = 0.0f;

    float processSample (
        float input,
        bool rightChannel);

    float getTargetMidiNote (
        float detectedMidi) const;

    bool isNoteAllowed (
        int midiNote) const;

    float midiToFrequency (
        float midi) const;

    float frequencyToMidi (
        float frequency) const;

    float semitoneDistance (
        float from,
        float to) const;

    float getRetuneCoefficient() const;

    float readDelay (
        const std::array<float, delaySize>& delay,
        float position) const;
};
