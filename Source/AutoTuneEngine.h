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

    void prepare (
        double newSampleRate,
        int newSamplesPerBlock);

    void reset();

    void setEnabled (bool shouldBeEnabled);
    void setKey (int newKey);
    void setScale (ScaleType newScale);
    void setRetuneSpeed (float newSpeed);
    void setAmount (float newAmount);

    void process (
        juce::AudioBuffer<float>& buffer);

private:

    // ============================================================
    // REALTIME PITCH SHIFTER
    //
    // Two overlapping granular read heads are used for the first
    // realtime correction stage.
    // ============================================================

    static constexpr int delaySize = 8192;
    static constexpr int grainSize = 256;

    struct Grain
    {
        float readPosition = 0.0f;
        int age = grainSize / 2;
        bool active = false;
    };

    struct ChannelState
    {
        std::array<float, delaySize> delay {};

        int writePosition = 0;

        Grain grainA;
        Grain grainB;
    };

    float processSample (
        float input,
        bool rightChannel);

    float processPitchSample (
        float input,
        ChannelState& channel,
        float pitchRatio);

    float readDelay (
        const std::array<float, delaySize>& delay,
        float position) const;

    float getGrainWindow (
        int age) const;

    void resetGrain (
        Grain& grain,
        const ChannelState& channel,
        float pitchRatio);

    // ============================================================
    // NOTE / PITCH CALCULATION
    // ============================================================

    float frequencyToMidi (
        float frequency) const;

    float midiToFrequency (
        float midi) const;

    bool isNoteAllowed (
        int midiNote) const;

    float getTargetMidiNote (
        float detectedMidi) const;

    float semitoneDistance (
        float from,
        float to) const;

    float getRetuneCoefficient() const;

    // ============================================================
    // PITCH DETECTION / CORRECTION STATE
    // ============================================================

    RealtimePitchDetector detector;

    double sampleRate = 44100.0;
    int blockSize = 128;

    bool enabled = false;

    int key = 0;

    ScaleType scale = ScaleType::Major;

    float retuneSpeed = 50.0f;
    float amount = 100.0f;

    // ============================================================
    // STEREO CHANNEL STATE
    // ============================================================

    ChannelState leftChannel;
    ChannelState rightChannel;

    // ============================================================
    // CURRENT CORRECTION
    // ============================================================

    float currentPitch = 0.0f;
    float targetPitch = 0.0f;

    float correctionSemitones = 0.0f;
    float smoothedCorrection = 0.0f;
};
