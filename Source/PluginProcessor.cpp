#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr int modeCount = 3;
    constexpr int diffuserCount = 4;
    constexpr int resonatorCount = 4;

    // Different delay topologies for three clearly different spring personalities.
    constexpr float diffuserTimes[modeCount][diffuserCount] =
    {
        { 0.0047f, 0.0073f, 0.0109f, 0.0133f }, // CLASSIC
        { 0.0017f, 0.0029f, 0.0043f, 0.0061f }, // DRIP
        { 0.0091f, 0.0147f, 0.0203f, 0.0271f }  // DREAM
    };
    constexpr float resonatorTimes[modeCount][resonatorCount] =
    {
        { 0.0311f, 0.0377f, 0.0439f, 0.0539f },
        { 0.0127f, 0.0179f, 0.0247f, 0.0313f },
        { 0.0413f, 0.0497f, 0.0611f, 0.0719f }
    };
    constexpr float resonatorGains[modeCount][resonatorCount] =
    {
        { 0.30f, 0.25f, 0.21f, 0.17f },
        { 0.31f, 0.27f, 0.23f, 0.18f },
        { 0.25f, 0.22f, 0.19f, 0.16f }
    };

    int wrapIndex (int index, int size) noexcept
    {
        while (index < 0) index += size;
        while (index >= size) index -= size;
        return index;
    }

    size_t diffuserIndex (int channel, int mode, int stage) noexcept
    {
        return static_cast<size_t> (mode * 2 * diffuserCount + channel * diffuserCount + stage);
    }

    size_t resonatorIndex (int channel, int mode, int stage) noexcept
    {
        return static_cast<size_t> (mode * 2 * resonatorCount + channel * resonatorCount + stage);
    }
}

void FuturoSpringAudioProcessor::DelayLine::prepare (int delaySamples)
{
    data.assign (static_cast<size_t> (juce::jmax (2, delaySamples)), 0.0f);
    writeIndex = 0;
}
void FuturoSpringAudioProcessor::DelayLine::clear()
{
    std::fill (data.begin(), data.end(), 0.0f);
    writeIndex = 0;
}
float FuturoSpringAudioProcessor::DelayLine::read() const noexcept
{
    return data.empty() ? 0.0f : data[writeIndex];
}
void FuturoSpringAudioProcessor::DelayLine::write (float sample) noexcept
{
    if (data.empty()) return;
    data[writeIndex] = sample;
    if (++writeIndex >= data.size()) writeIndex = 0;
}

FuturoSpringAudioProcessor::FuturoSpringAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput ("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

FuturoSpringAudioProcessor::APVTS::ParameterLayout FuturoSpringAudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout p;
    p.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.25f));
    p.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "intensity", 1 }, "Intensity",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.55f));
    p.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "saturation", 1 }, "Vintage Drive",
        juce::NormalisableRange<float> (0.0f, 18.0f, 0.1f), 2.0f));
    p.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "character", 1 }, "Character",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.1f), 35.0f));
    p.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "tone", 1 }, "Tone / Damping",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.7f));
    p.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "mode", 1 }, "Mode",
        juce::StringArray { "CLASSIC", "FLEX", "DREAM" }, 0));
    return p;
}

bool FuturoSpringAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    return input == output && (input == juce::AudioChannelSet::mono()
                            || input == juce::AudioChannelSet::stereo());
}

void FuturoSpringAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    currentSampleRate = juce::jmax (8000.0, sampleRate);
    const float fs = static_cast<float> (currentSampleRate);

    for (int mode = 0; mode < numModes; ++mode)
        for (int channel = 0; channel < maxChannels; ++channel)
        {
            for (int stage = 0; stage < numDiffusers; ++stage)
            {
                const float offset = channel == 0 ? 0.0f : 0.00019f * static_cast<float> (stage + 1);
                diffusers[diffuserIndex (channel, mode, stage)].prepare (
                    juce::jmax (2, juce::roundToInt ((diffuserTimes[mode][stage] + offset) * fs)));
            }
            for (int stage = 0; stage < numResonators; ++stage)
            {
                const float offset = channel == 0 ? 0.0f : 0.00043f * static_cast<float> (stage + 1);
                const auto index = resonatorIndex (channel, mode, stage);
                resonators[index].prepare (
                    juce::jmax (2, juce::roundToInt ((resonatorTimes[mode][stage] + offset) * fs)));
                resonatorDamping[index] = 0.0f;
            }
        }

    dampingState.fill (0.0f);
    wetPresenceLowpassState.fill (0.0f);
    previousInput.fill (0.0f);
    // Short moving buffers support WOBBLE, DOUBLER/FLANGER and MAGIC voices.
    characterBufferSize = static_cast<size_t> (juce::jmax (512, juce::roundToInt (fs * 0.120f) + 8));
    for (auto& channel : characterBuffers) channel.assign (characterBufferSize, 0.0f);
    characterWritePosition = 0;
    characterPhase = 0.0f;
    characterLowpassState.fill (0.0f);
    flexModPhase = 0.0f;

    // Three milliseconds of lookahead with linked peak detection and smooth release.
    outputDelaySamples = static_cast<size_t> (juce::jmax (1, juce::roundToInt (fs * 0.003f)));
    for (auto& channel : outputDelayBuffers) channel.assign (outputDelaySamples, 0.0f);
    outputDelayWritePosition = 0;
    outputPeakEnvelope = 0.0f;
    outputPeakRelease = std::exp (-1.0f / (0.100f * fs));
    setLatencySamples (static_cast<int> (outputDelaySamples));

    smoothMix = parameters.getRawParameterValue ("mix")->load();
    smoothIntensity = parameters.getRawParameterValue ("intensity")->load();
    smoothSaturation = parameters.getRawParameterValue ("saturation")->load();
    smoothCharacter = parameters.getRawParameterValue ("character")->load() * 0.01f;
    smoothTone = parameters.getRawParameterValue ("tone")->load();
    smoothMode = parameters.getRawParameterValue ("mode")->load();
    parameterSmoothing = 1.0f - std::exp (-1.0f / (0.020f * fs));
}

void FuturoSpringAudioProcessor::releaseResources()
{
    for (auto& line : diffusers) line.clear();
    for (auto& line : resonators) line.clear();
    for (auto& channel : characterBuffers) std::fill (channel.begin(), channel.end(), 0.0f);
    characterLowpassState.fill (0.0f);
    for (auto& channel : outputDelayBuffers) std::fill (channel.begin(), channel.end(), 0.0f);
    outputPeakEnvelope = 0.0f;
    resonatorDamping.fill (0.0f);
    dampingState.fill (0.0f);
    wetPresenceLowpassState.fill (0.0f);
    previousInput.fill (0.0f);
}

float FuturoSpringAudioProcessor::processDiffuser (int channel, int mode, float input, float coefficient) noexcept
{
    float x = input;
    for (int stage = 0; stage < numDiffusers; ++stage)
    {
        auto& line = diffusers[diffuserIndex (channel, mode, stage)];
        const float delayed = line.read();
        const float y = delayed - coefficient * x;
        line.write (x + coefficient * y);
        x = y;
    }
    return x;
}

float FuturoSpringAudioProcessor::readCharacterDelay (int channel, float delaySamples) const noexcept
{
    if (characterBufferSize < 4) return 0.0f;
    delaySamples = juce::jlimit (1.0f, static_cast<float> (characterBufferSize - 2), delaySamples);
    const float readPosition = static_cast<float> (characterWritePosition) - delaySamples;
    const float flooredPosition = std::floor (readPosition);
    const int index0 = wrapIndex (static_cast<int> (flooredPosition), static_cast<int> (characterBufferSize));
    const int index1 = (index0 + 1) % static_cast<int> (characterBufferSize);
    const float fraction = readPosition - flooredPosition;
    const auto& data = characterBuffers[static_cast<size_t> (channel)];
    return data[static_cast<size_t> (index0)]
         + fraction * (data[static_cast<size_t> (index1)] - data[static_cast<size_t> (index0)]);
}

float FuturoSpringAudioProcessor::saturate (float input, float amount) const noexcept
{
    // Unity small-signal gain. Normalising tanh by tanh(gain) boosted quiet loop signals,
    // which could make the previous reverb regenerate itself instead of decaying.
    const float drive = juce::jlimit (0.0f, 1.0f, amount / 18.0f);
    if (drive < 0.0005f) return input;
    const float gain = 1.0f + 4.0f * drive;
    return std::tanh (input * gain) / gain;
}

float FuturoSpringAudioProcessor::saturateVintage (float input, float amount) const noexcept
{
    // A gently asymmetric soft clip adds even harmonics (especially 2nd harmonic),
    // a classic analogue colour. The small-signal slope remains close to unity.
    const float drive = juce::jlimit (0.0f, 1.0f, amount / 18.0f);
    if (drive < 0.0005f) return input;
    const float gain = 1.0f + 4.5f * drive;
    const float x = input * gain;
    const float asymmetry = 0.065f * drive;
    return std::tanh (x + asymmetry * x * x) / gain;
}

void FuturoSpringAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused (midi);
    juce::ScopedNoDenormals noDenormals;

    const int outputChannels = juce::jmin (maxChannels, getTotalNumOutputChannels());
    const int inputChannels = juce::jmin (maxChannels, getTotalNumInputChannels());
    const int numSamples = buffer.getNumSamples();
    if (inputChannels < 1 || outputChannels < 1 || characterBufferSize < 4 || outputDelaySamples < 1) return;
    for (int channel = inputChannels; channel < outputChannels; ++channel)
        buffer.clear (channel, 0, numSamples);

    const float targetMix = parameters.getRawParameterValue ("mix")->load();
    const float targetIntensity = parameters.getRawParameterValue ("intensity")->load();
    const float targetSaturation = parameters.getRawParameterValue ("saturation")->load();
    const float targetCharacter = parameters.getRawParameterValue ("character")->load() * 0.01f;
    const float targetTone = parameters.getRawParameterValue ("tone")->load();
    const float targetMode = parameters.getRawParameterValue ("mode")->load();
    const float fs = static_cast<float> (currentSampleRate);
    const float inputLPAlpha = 1.0f - std::exp (-twoPi * 2300.0f / fs);
    const float wetPresenceAlpha = 1.0f - std::exp (-twoPi * 1300.0f / fs);

    for (int n = 0; n < numSamples; ++n)
    {
        smoothMix += parameterSmoothing * (targetMix - smoothMix);
        smoothIntensity += parameterSmoothing * (targetIntensity - smoothIntensity);
        smoothSaturation += parameterSmoothing * (targetSaturation - smoothSaturation);
        smoothCharacter += parameterSmoothing * (targetCharacter - smoothCharacter);
        smoothTone += parameterSmoothing * (targetTone - smoothTone);
        smoothMode += parameterSmoothing * (targetMode - smoothMode);

        // Slow, shallow feedback modulation gives FLEX a moving, elastic spring-bounce.
        const float flexMotion = std::sin (flexModPhase) * (0.020f + 0.055f * smoothIntensity);
        flexModPhase += twoPi * (0.38f + 0.40f * smoothIntensity) / fs;
        if (flexModPhase >= twoPi)
            flexModPhase -= twoPi;

        const float dryL = buffer.getSample (0, n);
        const float dryR = inputChannels > 1 ? buffer.getSample (1, n) : dryL;
        const float shaped[2] { saturate (dryL, smoothSaturation), saturate (dryR, smoothSaturation) };

        // Separate banks run concurrently so changing modes crossfades between distinct
        // delay networks and does not jump their internal delay-line state.
        const float modePosition = juce::jlimit (0.0f, 2.0f, smoothMode);
        const float modeWeights[numModes]
        {
            modePosition < 1.0f ? 1.0f - modePosition : 0.0f,
            modePosition < 1.0f ? modePosition : 2.0f - modePosition,
            modePosition > 1.0f ? modePosition - 1.0f : 0.0f
        };

        float wetTailByMode[numModes][maxChannels] {};
        float wetEarlyByMode[numModes][maxChannels] {};
        for (int channel = 0; channel < inputChannels; ++channel)
        {
            dampingState[static_cast<size_t> (channel)] += inputLPAlpha
                * (shaped[channel] - dampingState[static_cast<size_t> (channel)]);
            const float sourceLow = dampingState[static_cast<size_t> (channel)];
            const float sourceHigh = shaped[channel] - sourceLow;
            // This fast edge cue is used only by FLEX to give picked notes a springy snap.
            const float transient = shaped[channel] - previousInput[static_cast<size_t> (channel)];
            previousInput[static_cast<size_t> (channel)] = shaped[channel];

            for (int mode = 0; mode < numModes; ++mode)
            {
                const float source = mode == 0 ? shaped[channel]
                    : mode == 1 ? (0.50f * shaped[channel] + 1.08f * sourceHigh + 0.14f * transient)
                                : (0.62f * sourceLow + 0.38f * shaped[channel]);
                const float diffuserCoefficient = mode == 0 ? 0.68f : mode == 1 ? 0.39f : 0.79f;
                const float diffused = processDiffuser (channel, mode, source, diffuserCoefficient);

                // All loop gains are bounded. Even at maximum INTENSITY the per-loop
                // gain stays comfortably below unity, so tails decay instead of building.
                const float feedback = mode == 0 ? (0.52f + 0.32f * smoothIntensity)
                                     : mode == 1 ? (0.50f + 0.34f * smoothIntensity)
                                                 : (0.56f + 0.27f * smoothIntensity);
                const float excitation = mode == 0 ? 0.24f : mode == 1 ? 0.38f : 0.19f;
                const float highFeedback = mode == 0 ? 0.38f
                                         : mode == 1 ? juce::jlimit (0.78f, 0.95f, 0.86f + flexMotion)
                                                     : 0.16f;
                const float crossFeedback = mode == 0 ? 0.035f : mode == 1 ? 0.095f : 0.045f;
                const float toneBase = mode == 0 ? 600.0f : mode == 1 ? 800.0f : 360.0f;
                const float toneSpan = mode == 0 ? 10800.0f : mode == 1 ? 14000.0f : 4200.0f;
                const float cutoff = juce::jlimit (350.0f, fs * 0.44f, toneBase + toneSpan * smoothTone);
                const float toneAlpha = 1.0f - std::exp (-twoPi * cutoff / fs);

                float delayed[numResonators] {};
                float low[numResonators] {};
                float high[numResonators] {};
                float sumDelayed = 0.0f;
                for (int stage = 0; stage < numResonators; ++stage)
                {
                    const auto index = resonatorIndex (channel, mode, stage);
                    delayed[stage] = resonators[index].read();
                    resonatorDamping[index] += toneAlpha * (delayed[stage] - resonatorDamping[index]);
                    low[stage] = resonatorDamping[index];
                    high[stage] = delayed[stage] - low[stage];
                    sumDelayed += delayed[stage];
                }

                float output = 0.0f;
                for (int stage = 0; stage < numResonators; ++stage)
                {
                    const float otherResonators = (sumDelayed - delayed[stage]) / 3.0f;
                    const float feedbackSignal = low[stage] + high[stage] * highFeedback;
                    const float loopInput = diffused * excitation
                        + feedback * feedbackSignal
                        + crossFeedback * otherResonators;
                    const float boundedLoopInput = juce::jlimit (-1.5f, 1.5f, loopInput);
                    resonators[resonatorIndex (channel, mode, stage)].write (
                        mode == 0 ? saturateVintage (boundedLoopInput, 3.5f + smoothSaturation * 1.1f)
                                  : saturate (boundedLoopInput, smoothSaturation * (mode == 1 ? 0.60f : 0.45f)));
                    output += delayed[stage] * resonatorGains[mode][stage];
                }

                // Bring the spring tail forward without increasing loop feedback,
                // which previously made the reverb accumulate and turn muddy.
                // Presence comes from excitation and tone, not brute-force output gain.
                const float modeGain = mode == 0 ? (1.32f + 0.28f * smoothIntensity)
                                      : mode == 1 ? (1.48f + 0.30f * smoothIntensity)
                                                  : (1.08f + 0.20f * smoothIntensity);
                const float earlyGain = mode == 0 ? 0.20f : mode == 1 ? 0.21f : 0.23f;
                const float tail = output * modeGain;
                wetTailByMode[mode][channel] = mode == 0
                    ? saturateVintage (tail, 4.0f + smoothSaturation * 1.1f)
                    : saturate (tail, smoothSaturation * (mode == 1 ? 0.45f : 0.35f));
                const float early = diffused * earlyGain;
                wetEarlyByMode[mode][channel] = mode == 0
                    ? saturateVintage (early, 2.5f + smoothSaturation * 0.65f)
                    : saturate (early, smoothSaturation * 0.25f);
            }
        }

        float wetTail[2] {};
        float wetEarly[2] {};
        for (int channel = 0; channel < inputChannels; ++channel)
            for (int mode = 0; mode < numModes; ++mode)
            {
                wetTail[channel] += wetTailByMode[mode][channel] * modeWeights[mode];
                wetEarly[channel] += wetEarlyByMode[mode][channel] * modeWeights[mode];
            }

        // Keep a clean copy for modes whose character is modulation-only.
        // FLEX intentionally stores processed output to create a controlled elastic tail.
        float wet[2] { wetEarly[0] + wetTail[0], wetEarly[1] + wetTail[1] };
        const float characterInputWet[2] { wet[0], wet[1] };

        const float character = juce::jlimit (0.0f, 1.0f, smoothCharacter);
        const float characterRate = modePosition < 0.5f ? (0.20f + 0.42f * character)
                                   : modePosition < 1.5f ? (0.16f + 0.27f * character)
                                                         : (0.055f + 0.13f * character);

        if (modePosition < 0.5f)
        {
            // CLASSIC / WOBBLE: slow, tape-like modulation in the wet field.
            for (int channel = 0; channel < inputChannels; ++channel)
            {
                const float phase = characterPhase + static_cast<float> (channel) * 0.90f;
                const float delay = fs * (0.015f + std::sin (phase) * 0.008f * character);
                const float moving = readCharacterDelay (channel, delay);
                const float amount = character * (0.48f + 0.24f * character);
                wet[channel] = wet[channel] * (1.0f - 0.42f * amount) + moving * amount;
                wet[channel] *= 1.0f + 0.10f * character * std::sin (phase + 0.7f);
            }
        }
        else if (modePosition < 1.5f)
        {
            // FLEX / BLOOM: an elastic spring rebound instead of a doubler/flanger.
            // A modulated 25-65ms feedback path blooms into quieter rebounds.
            // Its loop coefficient stays below 0.88, allowing the repeats to decay.
            for (int channel = 0; channel < inputChannels; ++channel)
            {
                const float phase = characterPhase + static_cast<float> (channel) * 0.72f;
                const float delay = fs * (0.045f + 0.020f * character * std::sin (phase));
                const float own = readCharacterDelay (channel, delay);
                const float other = inputChannels > 1
                    ? readCharacterDelay (1 - channel, delay * 0.96f) : own;
                const float rebound = own * 0.84f + other * 0.16f;
                const float feedback = character * (0.56f + 0.32f * character);
                wet[channel] = wet[channel] * (1.0f - 0.20f * character)
                             + rebound * feedback;
            }
        }
        else
        {
            // DREAM / MAGIC: nostalgic tape memory with a slow, dark, cross-fed halo.
            for (int channel = 0; channel < inputChannels; ++channel)
            {
                const float phase = characterPhase + static_cast<float> (channel) * 1.13f;
                const float delay = fs * (0.060f + 0.024f * character * std::sin (phase)
                                          + static_cast<float> (channel) * 0.008f);
                const float own = readCharacterDelay (channel, delay);
                const float other = inputChannels > 1
                    ? readCharacterDelay (1 - channel, delay * 0.91f) : own;
                const float memory = own * 0.72f + other * 0.28f;
                const float cutoff = 3400.0f - 1900.0f * character;
                const float alpha = 1.0f - std::exp (-twoPi * cutoff / fs);
                characterLowpassState[static_cast<size_t> (channel)] += alpha
                    * (memory - characterLowpassState[static_cast<size_t> (channel)]);
                wet[channel] = wet[channel] * (1.0f - 0.22f * character)
                             + characterLowpassState[static_cast<size_t> (channel)] * (0.72f * character);
            }
        }

        characterPhase += twoPi * characterRate / fs;
        if (characterPhase >= twoPi)
            characterPhase -= twoPi;

        // Capture the feedback signal before the output tone shelf. This keeps TONE
        // from multiplying the feedback loop and preserves a decaying, clean tail.
        const float characterFeedbackWet[2] { wet[0], wet[1] };

        // DREAM widens only the recombined wet field while keeping the centre stable.
        if (inputChannels > 1 && modePosition > 1.0f)
        {
            const float mid = 0.5f * (wet[0] + wet[1]);
            const float side = 0.5f * (wet[0] - wet[1]);
            const float width = 1.0f + 0.48f * (modePosition - 1.0f);
            wet[0] = mid + side * width;
            wet[1] = mid - side * width;
        }

        // Gentle wet-only high shelf adds articulation and presence. TONE also
        // determines how far the upper part of the reverb steps forward.
        for (int channel = 0; channel < inputChannels; ++channel)
        {
            auto& low = wetPresenceLowpassState[static_cast<size_t> (channel)];
            low += wetPresenceAlpha * (wet[channel] - low);
            // Musical dark-to-bright shelf with limited high-band boost for headroom.
            const float highBandGain = 0.28f + 1.18f * std::pow (smoothTone, 1.10f);
            wet[channel] = low + (wet[channel] - low) * highBandGain;
        }

        // FLEX feeds its processed output back into the character buffer; other modes
        // store the clean spring field to keep their movement effects from self-building.
        for (int channel = 0; channel < inputChannels; ++channel)
            characterBuffers[static_cast<size_t> (channel)][characterWritePosition]
                = (modePosition >= 0.5f && modePosition < 1.5f) ? characterFeedbackWet[channel] : characterInputWet[channel];

        const float wetL = wet[0];
        const float wetR = inputChannels > 1 ? wet[1] : wetL;
        const float mix = juce::jlimit (0.0f, 1.0f, smoothMix);
        const float dryGain = std::cos (0.5f * juce::MathConstants<float>::pi * mix);
        const float wetGain = std::sin (0.5f * juce::MathConstants<float>::pi * mix);
        const float rawOutL = dryL * dryGain + wetL * wetGain;
        const float rawOutR = dryR * dryGain + wetR * wetGain;

        // Linked, lookahead peak control prevents clipping without adding saturation.
        const float peakNow = juce::jmax (std::abs (rawOutL), std::abs (rawOutR));
        outputPeakEnvelope = juce::jmax (peakNow, outputPeakEnvelope * outputPeakRelease);
        constexpr float outputCeiling = 0.96f;
        const float limiterGain = outputPeakEnvelope > outputCeiling
                                ? outputCeiling / outputPeakEnvelope : 1.0f;
        const float delayedL = outputDelayBuffers[0][outputDelayWritePosition];
        const float delayedR = outputDelayBuffers[1][outputDelayWritePosition];
        outputDelayBuffers[0][outputDelayWritePosition] = rawOutL;
        outputDelayBuffers[1][outputDelayWritePosition] = rawOutR;
        buffer.setSample (0, n, delayedL * limiterGain);
        if (outputChannels > 1) buffer.setSample (1, n, delayedR * limiterGain);

        if (++outputDelayWritePosition >= outputDelaySamples) outputDelayWritePosition = 0;
        if (++characterWritePosition >= characterBufferSize) characterWritePosition = 0;
    }
}

juce::AudioProcessorEditor* FuturoSpringAudioProcessor::createEditor()
{
    return new FuturoSpringAudioProcessorEditor (*this);
}

void FuturoSpringAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = parameters.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void FuturoSpringAudioProcessor::setStateInformation (const void* data, int size)
{
    if (data == nullptr || size <= 0) return;
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FuturoSpringAudioProcessor();
}
