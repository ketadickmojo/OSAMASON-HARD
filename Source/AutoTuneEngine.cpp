#include "AutoTuneEngine.h"

// ================================================================
// PREPARE
// ================================================================

void AutoTuneEngine::prepare (
    double newSampleRate,
    int newSamplesPerBlock)
{
    sampleRate = newSampleRate;

    if (sampleRate <= 0.0)
        sampleRate = 44100.0;

    blockSize = newSamplesPerBlock;

    if (blockSize <= 0)
        blockSize = 128;

    detector.prepare (sampleRate);

    reset();
}

// ================================================================
// RESET
// ================================================================

void AutoTuneEngine::reset()
{
    detector.reset();

    leftChannel.delay.fill (0.0f);
    rightChannel.delay.fill (0.0f);

    leftChannel.writePosition = 0;
    rightChannel.writePosition = 0;

    leftChannel.grainA = {};
    leftChannel.grainB = {};

    rightChannel.grainA = {};
    rightChannel.grainB = {};

    leftChannel.grainA.age = grainSize / 2;
    leftChannel.grainB.age = 0;

    rightChannel.grainA.age = grainSize / 2;
    rightChannel.grainB.age = 0;

    currentPitch = 0.0f;
    targetPitch = 0.0f;

    correctionSemitones = 0.0f;
    smoothedCorrection = 0.0f;
}

// ================================================================
// ENABLE
// ================================================================

void AutoTuneEngine::setEnabled (
    bool shouldBeEnabled)
{
    if (enabled == shouldBeEnabled)
        return;

    enabled = shouldBeEnabled;

    if (!enabled)
        reset();
}

// ================================================================
// KEY
// ================================================================

void AutoTuneEngine::setKey (
    int newKey)
{
    key =
        juce::jlimit (
            0,
            11,
            newKey);
}

// ================================================================
// SCALE
// ================================================================

void AutoTuneEngine::setScale (
    ScaleType newScale)
{
    scale = newScale;
}

// ================================================================
// RETUNE SPEED
// ================================================================

void AutoTuneEngine::setRetuneSpeed (
    float newSpeed)
{
    retuneSpeed =
        juce::jlimit (
            0.0f,
            100.0f,
            newSpeed);
}

// ================================================================
// AMOUNT
// ================================================================

void AutoTuneEngine::setAmount (
    float newAmount)
{
    amount =
        juce::jlimit (
            0.0f,
            100.0f,
            newAmount);
}

// ================================================================
// FREQUENCY -> MIDI
// ================================================================

float AutoTuneEngine::frequencyToMidi (
    float frequency) const
{
    if (frequency <= 0.0f)
        return -1.0f;

    return
        69.0f
        + 12.0f
        * std::log2 (
            frequency / 440.0f);
}

// ================================================================
// MIDI -> FREQUENCY
// ================================================================

float AutoTuneEngine::midiToFrequency (
    float midi) const
{
    return
        440.0f
        * std::pow (
            2.0f,
            (midi - 69.0f) / 12.0f);
}

// ================================================================
// NOTE ALLOWED BY SCALE
// ================================================================

bool AutoTuneEngine::isNoteAllowed (
    int midiNote) const
{
    if (scale == ScaleType::Chromatic)
        return true;

    static constexpr bool major[12] =
    {
        true,
        false,
        true,
        false,
        true,
        true,
        false,
        true,
        false,
        true,
        false,
        true
    };

    static constexpr bool minor[12] =
    {
        true,
        false,
        true,
        true,
        false,
        true,
        false,
        true,
        true,
        false,
        true,
        false
    };

    const int pitchClass =
        ((midiNote % 12) + 12) % 12;

    const int relative =
        ((pitchClass - key) + 12) % 12;

    if (scale == ScaleType::Major)
        return major[relative];

    return minor[relative];
}

// ================================================================
// FIND TARGET NOTE
// ================================================================

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
                detectedMidi
                - static_cast<float> (
                    note));

        if (distance < bestDistance)
        {
            bestDistance = distance;
            bestNote = static_cast<float> (note);
        }
    }

    return bestNote;
}

// ================================================================
// SEMITONE DISTANCE
// ================================================================

float AutoTuneEngine::semitoneDistance (
    float from,
    float to) const
{
    return to - from;
}

// ================================================================
// RETUNE COEFFICIENT
// ================================================================

float AutoTuneEngine::getRetuneCoefficient() const
{
    const float normalized =
        juce::jlimit (
            0.0f,
            1.0f,
            retuneSpeed / 100.0f);

    return
        0.0025f
        + normalized * 0.30f;
}

// ================================================================
// DELAY READ
// ================================================================

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
        (index0 + 1)
        % delaySize;

    const float frac =
        position
        - static_cast<float> (
            index0);

    return
        delay[static_cast<std::size_t> (index0)]
        * (1.0f - frac)
        + delay[static_cast<std::size_t> (index1)]
        * frac;
}

// ================================================================
// GRAIN WINDOW
// ================================================================

float AutoTuneEngine::getGrainWindow (
    int age) const
{
    if (age < 0 || age >= grainSize)
        return 0.0f;

    const float phase =
        static_cast<float> (age)
        / static_cast<float> (grainSize - 1);

    return
        0.5f
        - 0.5f
        * std::cos (
            2.0f
            * juce::MathConstants<float>::pi
            * phase);
}

// ================================================================
// RESET GRAIN
// ================================================================

void AutoTuneEngine::resetGrain (
    Grain& grain,
    const ChannelState& channel,
    float pitchRatio)
{
    const float baseDelay =
        static_cast<float> (
            grainSize);

    float startPosition =
        static_cast<float> (
            channel.writePosition)
        - baseDelay;

    startPosition -=
        (pitchRatio - 1.0f)
        * static_cast<float> (
            grainSize)
        * 0.25f;

    while (startPosition < 0.0f)
        startPosition +=
            static_cast<float> (
                delaySize);

    while (startPosition >=
           static_cast<float> (
               delaySize))
    {
        startPosition -=
            static_cast<float> (
                delaySize);
    }

    grain.readPosition =
        startPosition;

    grain.age = 0;
    grain.active = true;
}

// ================================================================
// PITCH PROCESSING
// ================================================================

float AutoTuneEngine::processPitchSample (
    float input,
    ChannelState& channel,
    float pitchRatio)
{
    channel.delay[
        static_cast<std::size_t> (
            channel.writePosition)] =
        input;

    pitchRatio =
        juce::jlimit (
            0.5f,
            2.0f,
            pitchRatio);

    if (!channel.grainA.active)
    {
        resetGrain (
            channel.grainA,
            channel,
            pitchRatio);
    }

    if (!channel.grainB.active)
    {
        resetGrain (
            channel.grainB,
            channel,
            pitchRatio);
    }

    float output = 0.0f;
    float windowSum = 0.0f;

    // ------------------------------------------------------------
    // GRAIN A
    // ------------------------------------------------------------

    if (channel.grainA.active)
    {
        const float window =
            getGrainWindow (
                channel.grainA.age);

        output +=
            readDelay (
                channel.delay,
                channel.grainA.readPosition)
            * window;

        windowSum += window;

        channel.grainA.readPosition +=
            pitchRatio;

        if (channel.grainA.readPosition >=
            static_cast<float> (
                delaySize))
        {
            channel.grainA.readPosition -=
                static_cast<float> (
                    delaySize);
        }

        ++channel.grainA.age;

        if (channel.grainA.age >= grainSize)
            channel.grainA.active = false;
    }

    // ------------------------------------------------------------
    // GRAIN B
    // ------------------------------------------------------------

    if (channel.grainB.active)
    {
        const float window =
            getGrainWindow (
                channel.grainB.age);

        output +=
            readDelay (
                channel.delay,
                channel.grainB.readPosition)
            * window;

        windowSum += window;

        channel.grainB.readPosition +=
            pitchRatio;

        if (channel.grainB.readPosition >=
            static_cast<float> (
                delaySize))
        {
            channel.grainB.readPosition -=
                static_cast<float> (
                    delaySize);
        }

        ++channel.grainB.age;

        if (channel.grainB.age >= grainSize)
            channel.grainB.active = false;
    }

    // ------------------------------------------------------------
    // NORMALIZE OVERLAP
    // ------------------------------------------------------------

    if (windowSum > 1.0e-5f)
    {
        output /= windowSum;
    }
    else
    {
        output = input;
    }

    // ------------------------------------------------------------
    // RESTART INACTIVE GRAIN
    // ------------------------------------------------------------

    if (!channel.grainA.active
        && channel.grainB.active)
    {
        resetGrain (
            channel.grainA,
            channel,
            pitchRatio);
    }

    if (!channel.grainB.active
        && channel.grainA.active)
    {
        resetGrain (
            channel.grainB,
            channel,
            pitchRatio);
    }

    ++channel.writePosition;

    if (channel.writePosition >= delaySize)
        channel.writePosition = 0;

    return output;
}

// ================================================================
// PROCESS ONE SAMPLE
// ================================================================

float AutoTuneEngine::processSample (
    float input,
    bool isRightChannel)
{
    // IMPORTANT:
    // The parameter is called isRightChannel so it does not
    // conflict with the rightChannel member variable.

    auto& channel =
        isRightChannel
            ? rightChannel
            : leftChannel;

    // ------------------------------------------------------------
    // PITCH DETECTION
    // ------------------------------------------------------------

    const float detected =
        detector.process (
            input);

    // Update pitch/correction only once per stereo sample pair.
    if (!isRightChannel && detected > 0.0f)
    {
        currentPitch =
            detected;

        const float detectedMidi =
            frequencyToMidi (
                currentPitch);

        targetPitch =
            getTargetMidiNote (
                detectedMidi);
    }

    // ------------------------------------------------------------
    // NO VALID PITCH
    // ------------------------------------------------------------

    if (currentPitch <= 0.0f
        || targetPitch <= 0.0f)
    {
        return input;
    }

    // ------------------------------------------------------------
    // CALCULATE CORRECTION
    // ------------------------------------------------------------

    const float detectedMidi =
        frequencyToMidi (
            currentPitch);

    const float desiredSemitones =
        semitoneDistance (
            detectedMidi,
            targetPitch);

    const float amountNormalized =
        juce::jlimit (
            0.0f,
            1.0f,
            amount / 100.0f);

    const float amountScaled =
        desiredSemitones
        * amountNormalized;

    // ------------------------------------------------------------
    // RETUNE SPEED
    // ------------------------------------------------------------

    const float coefficient =
        getRetuneCoefficient();

    smoothedCorrection +=
        (amountScaled - smoothedCorrection)
        * coefficient;

    correctionSemitones =
        smoothedCorrection;

    // ------------------------------------------------------------
    // SEMITONES -> RATIO
    // ------------------------------------------------------------

    const float pitchRatio =
        std::pow (
            2.0f,
            correctionSemitones / 12.0f);

    // ------------------------------------------------------------
    // REALTIME PITCH SHIFT
    // ------------------------------------------------------------

    return
        processPitchSample (
            input,
            channel,
            pitchRatio);
}

// ================================================================
// PROCESS BLOCK
// ================================================================

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
