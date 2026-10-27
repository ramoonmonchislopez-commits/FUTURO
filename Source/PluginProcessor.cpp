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
    p.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "saturation", 1 }, "Saturation",
        juce::NormalisableRange<float> (0.0f, 18.0f, 0.1f), 2.0f));
    p.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "pitch", 1 }, "Pitch",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));
    p.add (std::make_unique<juce::AudioParameterFloat> (juce::ParameterID { "tone", 1 }, "Tone",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.7f));
    p.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "mode", 1 }, "Mode",
        juce::StringArray { "CLASSIC", "DRIP", "DREAM" }, 0));
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
    minimumPitchDelaySamples = fs * 0.020f;
    pitchSweepRangeSamples = fs * 0.090f;
    pitchBufferSize = static_cast<size_t> (juce::jmax (512, juce::roundToInt (fs * 0.140f) + 8));
    for (auto& channel : pitchBuffers) channel.assign (pitchBufferSize, 0.0f);
    pitchWritePosition = 0;
    pitchPhase = 0.0f;
    pitchWetBlend = 0.0f;

    smoothMix = parameters.getRawParameterValue ("mix")->load();
    smoothIntensity = parameters.getRawParameterValue ("intensity")->load();
    smoothSaturation = parameters.getRawParameterValue ("saturation")->load();
    smoothPitch = parameters.getRawParameterValue ("pitch")->load();
    smoothTone = parameters.getRawParameterValue ("tone")->load();
    smoothMode = parameters.getRawParameterValue ("mode")->load();
    parameterSmoothing = 1.0f - std::exp (-1.0f / (0.020f * fs));
}

void FuturoSpringAudioProcessor::releaseResources()
{
    for (auto& line : diffusers) line.clear();
    for (auto& line : resonators) line.clear();
    for (auto& channel : pitchBuffers) std::fill (channel.begin(), channel.end(), 0.0f);
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

float FuturoSpringAudioProcessor::readPitchDelay (int channel, float delaySamples) const noexcept
{
    if (pitchBufferSize < 4) return 0.0f;
    delaySamples = juce::jlimit (1.0f, static_cast<float> (pitchBufferSize - 2), delaySamples);
    const float readPosition = static_cast<float> (pitchWritePosition) - delaySamples;
    const float flooredPosition = std::floor (readPosition);
    const int index0 = wrapIndex (static_cast<int> (flooredPosition), static_cast<int> (pitchBufferSize));
    const int index1 = (index0 + 1) % static_cast<int> (pitchBufferSize);
    const float fraction = readPosition - flooredPosition;
    const auto& data = pitchBuffers[static_cast<size_t> (channel)];
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

void FuturoSpringAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused (midi);
    juce::ScopedNoDenormals noDenormals;

    const int outputChannels = juce::jmin (maxChannels, getTotalNumOutputChannels());
    const int inputChannels = juce::jmin (maxChannels, getTotalNumInputChannels());
    const int numSamples = buffer.getNumSamples();
    if (inputChannels < 1 || outputChannels < 1 || pitchBufferSize < 4) return;
    for (int channel = inputChannels; channel < outputChannels; ++channel)
        buffer.clear (channel, 0, numSamples);

    const float targetMix = parameters.getRawParameterValue ("mix")->load();
    const float targetIntensity = parameters.getRawParameterValue ("intensity")->load();
    const float targetSaturation = parameters.getRawParameterValue ("saturation")->load();
    const float targetPitch = parameters.getRawParameterValue ("pitch")->load();
    const float targetTone = parameters.getRawParameterValue ("tone")->load();
    const float targetMode = parameters.getRawParameterValue ("mode")->load();
    const float fs = static_cast<float> (currentSampleRate);
    const float inputLPAlpha = 1.0f - std::exp (-twoPi * 2300.0f / fs);
    const float wetPresenceAlpha = 1.0f - std::exp (-twoPi * 1900.0f / fs);

    for (int n = 0; n < numSamples; ++n)
    {
        smoothMix += parameterSmoothing * (targetMix - smoothMix);
        smoothIntensity += parameterSmoothing * (targetIntensity - smoothIntensity);
        smoothSaturation += parameterSmoothing * (targetSaturation - smoothSaturation);
        smoothPitch += parameterSmoothing * (targetPitch - smoothPitch);
        smoothTone += parameterSmoothing * (targetTone - smoothTone);
        smoothMode += parameterSmoothing * (targetMode - smoothMode);

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

        float wetByMode[numModes][maxChannels] {};
        for (int channel = 0; channel < inputChannels; ++channel)
        {
            dampingState[static_cast<size_t> (channel)] += inputLPAlpha
                * (shaped[channel] - dampingState[static_cast<size_t> (channel)]);
            const float sourceLow = dampingState[static_cast<size_t> (channel)];
            const float sourceHigh = shaped[channel] - sourceLow;

            for (int mode = 0; mode < numModes; ++mode)
            {
                const float source = mode == 0 ? shaped[channel]
                    : mode == 1 ? (0.62f * shaped[channel] + 0.95f * sourceHigh)
                                : (0.62f * sourceLow + 0.38f * shaped[channel]);
                const float diffuserCoefficient = mode == 0 ? 0.68f : mode == 1 ? 0.39f : 0.79f;
                const float diffused = processDiffuser (channel, mode, source, diffuserCoefficient);

                // All loop gains are bounded. Even at maximum INTENSITY the per-loop
                // gain stays comfortably below unity, so tails decay instead of building.
                const float feedback = mode == 0 ? (0.48f + 0.28f * smoothIntensity)
                                     : mode == 1 ? (0.43f + 0.25f * smoothIntensity)
                                                 : (0.57f + 0.22f * smoothIntensity);
                const float excitation = mode == 0 ? 0.24f : mode == 1 ? 0.31f : 0.17f;
                const float highFeedback = mode == 0 ? 0.38f : mode == 1 ? 0.96f : 0.12f;
                const float crossFeedback = mode == 0 ? 0.035f : mode == 1 ? 0.055f : 0.020f;
                const float toneBase = mode == 0 ? 1350.0f : mode == 1 ? 4200.0f : 650.0f;
                const float toneSpan = mode == 0 ? 10500.0f : mode == 1 ? 12000.0f : 4400.0f;
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
                    resonators[resonatorIndex (channel, mode, stage)].write (
                        saturate (juce::jlimit (-1.5f, 1.5f, loopInput), smoothSaturation * 0.45f));
                    output += delayed[stage] * resonatorGains[mode][stage];
                }

                // Bring the spring tail forward without increasing loop feedback,
                // which previously made the reverb accumulate and turn muddy.
                const float modeGain = mode == 0 ? (1.12f + 0.25f * smoothIntensity)
                                      : mode == 1 ? (1.28f + 0.30f * smoothIntensity)
                                                  : (0.91f + 0.18f * smoothIntensity);
                const float earlyGain = mode == 0 ? 0.20f : mode == 1 ? 0.13f : 0.23f;
                wetByMode[mode][channel] = saturate (
                    output * modeGain + diffused * earlyGain,
                    smoothSaturation * 0.35f);
            }
        }

        float wet[2] {};
        for (int channel = 0; channel < inputChannels; ++channel)
            for (int mode = 0; mode < numModes; ++mode)
                wet[channel] += wetByMode[mode][channel] * modeWeights[mode];

        // Gentle wet-only high shelf adds articulation and presence. TONE also
        // determines how far the upper part of the reverb steps forward.
        for (int channel = 0; channel < inputChannels; ++channel)
        {
            auto& low = wetPresenceLowpassState[static_cast<size_t> (channel)];
            low += wetPresenceAlpha * (wet[channel] - low);
            wet[channel] += (wet[channel] - low) * (0.10f + 0.35f * smoothTone);
        }

        // Pitch processing is confined to the wet path.
        const float pitchEnabled = std::abs (smoothPitch) > 0.025f ? 1.0f : 0.0f;
        pitchWetBlend += parameterSmoothing * (pitchEnabled - pitchWetBlend);
        const float ratio = std::pow (2.0f, smoothPitch / 12.0f);
        const float phaseStep = std::abs (1.0f - ratio) / juce::jmax (1.0f, pitchSweepRangeSamples);
        for (int channel = 0; channel < inputChannels; ++channel)
            pitchBuffers[static_cast<size_t> (channel)][pitchWritePosition] = wet[channel];

        float shifted[2] { wet[0], wet[1] };
        if (pitchWetBlend > 0.0001f && std::abs (smoothPitch) > 0.025f)
        {
            const bool pitchUp = ratio > 1.0f;
            const float phaseA = pitchPhase;
            float phaseB = pitchPhase + 0.5f;
            if (phaseB >= 1.0f) phaseB -= 1.0f;
            const auto delayForPhase = [this, pitchUp] (float phase)
            {
                const float ramp = pitchUp ? (1.0f - phase) : phase;
                return minimumPitchDelaySamples + ramp * pitchSweepRangeSamples;
            };
            const float weightA = 0.5f - 0.5f * std::cos (twoPi * phaseA);
            const float weightB = 1.0f - weightA;
            for (int channel = 0; channel < inputChannels; ++channel)
            {
                const float a = readPitchDelay (channel, delayForPhase (phaseA));
                const float b = readPitchDelay (channel, delayForPhase (phaseB));
                shifted[channel] = a * weightA + b * weightB;
            }
            pitchPhase += phaseStep;
            if (pitchPhase >= 1.0f) pitchPhase -= std::floor (pitchPhase);
        }

        // Equal-power blend avoids perceptual dips and lets the wet signal stay audible
        // at low MIX settings while preserving an exact 100% wet position.
        const float mix = juce::jlimit (0.0f, 1.0f, smoothMix);
        const float dryGain = std::cos (0.5f * juce::MathConstants<float>::pi * mix);
        const float wetGain = std::sin (0.5f * juce::MathConstants<float>::pi * mix);
        const float wetL = wet[0] * (1.0f - pitchWetBlend) + shifted[0] * pitchWetBlend;
        const float wetR = inputChannels > 1
            ? wet[1] * (1.0f - pitchWetBlend) + shifted[1] * pitchWetBlend : wetL;
        buffer.setSample (0, n, dryL * dryGain + wetL * wetGain);
        if (outputChannels > 1) buffer.setSample (1, n, dryR * dryGain + wetR * wetGain);

        if (++pitchWritePosition >= pitchBufferSize) pitchWritePosition = 0;
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
