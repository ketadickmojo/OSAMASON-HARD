#include "AutoTuneEngine.h"

void AutoTuneEngine::prepare (
    double newSampleRate,
    int newSamplesPerBlock)
{
    sampleRate = newSampleRate;
    blockSize = newSamplesPerBlock;

    detector.prepare (sampleRate);

    reset();
}

void AutoTuneEngine::reset()
{
    detector.reset();

    delayL.fill (0.0f);
    delayR.fill (0.0f);

    delayWritePosition = 0;

    currentPitch = 0.0f;
    targetPitch = 0.0f;

    correctionSemitones = 0.0f;
    smoothedCorrection = 0.0f;

    oscillatorPhase = 0.0f;
}

void AutoTuneEngine::setEnabled (
    bool shouldBeEnabled)
{
    if (enabled == shouldBeEnabled)
        return;

    enabled = shouldBeEnabled;

    if (!enabled)
        reset();
}

void AutoTuneEngine::setKey (
    int newKey)
{
    key =
        juce::jlimit (
            0,
            11,
            newKey);
}

void AutoTuneEngine::setScale (
    ScaleType newScale)
{
    scale = newScale;
}

void AutoTuneEngine::setRetuneSpeed (
    float newSpeed)
{
    retuneSpeed =
        juce::jlimit (
            0.0f,
            100.0f,
            newSpeed);
}

void AutoTuneEngine::setAmount (
    float newAmount)
{
    amount =
        juce::jlimit (
            0.0f,
            100.0f,
            newAmount);
}

float AutoTuneEngine::frequencyToMidi (
    float frequency) const
{
    if (frequency <= 0.0f)
        return -1.0f;

    return
        69.0f +
        12.0f *
        std::log2 (
            frequency / 440.0f);
}

float AutoTuneEngine::midiToFrequency (
    float midi) const
{
    return
        440.0f *
        std::pow (
            2.0f,
            (midi - 69.0f) / 12.0f);
}

bool AutoTuneEngine::isNoteAllowed (
    int midiNote) const
{
    if (scale == ScaleType::Chromatic)
        return true;

    static constexpr bool major[12] =
    {
        true, false, true, false,
        true, true, false, true,
        false, true, false, true
    };

    static constexpr bool minor[12] =
    {
        true, false, true, true,
        false, true, false, true,
        true, false, true, false
    };

    const int pitchClass =
        ((midiNote % 12) + 12) % 12;

    const int relative =
        ((pitchClass - key) + 12) % 12;

    if (scale == ScaleType::Major)
        return major[relative];

    return minor[relative];
}

float AutoTuneEngine::getTargetMidiNote (
    float detectedMidi) const
{
    if (detectedMidi < 0.0f)
        return detectedMidi;

    const int center =
        static_cast<int> (
            std::round (
                detectedMidi));

    float bestNote =
        static_cast<float> (
            center);

    float bestDistance =
        std::numeric_limits<float>::max();

    for (int note = center - 12;
         note <= center + 12;
         ++note)
    {
        if (!isNoteAllowed (note))
            continue;

        const float distance =
            std::abs (
                detectedMidi -
                static_cast<float> (
                    note));

        if (distance < bestDistance)
        {
            bestDistance =
                distance;

            bestNote =
                static_cast<float> (
                    note);
        }
    }

    return bestNote;
}

float AutoTuneEngine::semitoneDistance (
    float from,
    float to) const
{
    return to - from;
}

float AutoTuneEngine::getRetuneCoefficient() const
{
    const float normalized =
        juce::jlimit (
            0.0f,
            1.0f,
            retuneSpeed / 100.0f);

    return
        0.0005f +
        normalized * 0.25f;
}

float AutoTuneEngine::readDelay (
    const std::array<float, delaySize>& delay,
    float position) const
{
    while (position < 0.0f)
        position +=
            static_cast<float> (
                delaySize);

    while (position >=
           static_cast<float> (
               delaySize))
    {
        position -=
            static_cast<float> (
                delaySize);
    }

    const int index0 =
        static_cast<int> (
            position);

    const int index1 =
        (index0 + 1) %
        delaySize;

    const float frac =
        position -
        static_cast<float> (
            index0);

    return
        delay[index0] *
            (1.0f - frac)
        +
        delay[index1] *
            frac;
}

float AutoTuneEngine::processSample (
    float input,
    bool rightChannel)
{
    auto& delay =
        rightChannel
            ? delayR
            : delayL;

    delay[delayWritePosition] =
        input;

    const float detected =
        detector.process (
            input);

    if (detected > 0.0f)
        currentPitch = detected;

    /*
        Пока не изменяем высоту звука.

        Этот этап нужен для проверки:
        - realtime pitch detector
        - Key
        - Scale
        - Retune Speed
        - Amount
        - сохранения состояния между блоками
    */

    delayWritePosition++;

    if (delayWritePosition >=
        delaySize)
    {
        delayWritePosition = 0;
    }

    return input;
}

void AutoTuneEngine::process (
    juce::AudioBuffer<float>& buffer)
{
    if (!enabled)
        return;

    const int numSamples =
        buffer.getNumSamples();

    const int numChannels =
        buffer.getNumChannels();

    if (numChannels <= 0)
        return;

    auto* left =
        buffer.getWritePointer (0);

    auto* right =
        numChannels > 1
            ? buffer.getWritePointer (1)
            : nullptr;

    for (int i = 0;
         i < numSamples;
         ++i)
    {
        left[i] =
            processSample (
                left[i],
                false);

        if (right != nullptr)
        {
            right[i] =
                processSample (
                    right[i],
                    true);
        }
    }
}
