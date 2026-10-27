#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr std::array<float, 4> diffuserTimesSeconds { 0.0047f, 0.0073f, 0.0109f, 0.0133f };
    constexpr std::array<float, 4> resonatorTimesSeconds { 0.0297f, 0.0371f, 0.0411f, 0.0533f };
    constexpr std::array<float, 4> resonatorGains { 0.29f, 0.27f, 0.25f, 0.23f };

    int wrapIndex (int i, int size) noexcept
    {
        while (i < 0)
            i += size;
        while (i >= size)
            i -= size;
        return i;
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
    if (data.empty())
        return;
    data[writeIndex] = sample;
    if (++writeIndex >= data.size())
        writeIndex = 0;
}

FuturoSpringAudioProcessor::FuturoSpringAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

FuturoSpringAudioProcessor::APVTS::ParameterLayout FuturoSpringAudioProcessor::createParameterLayout()
{
    APVTS::ParameterLayout p;
    p.add (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.25f));
    p.add (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "intensity", 1 }, "Intensity",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.55f));
    p.add (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "saturation", 1 }, "Saturation",
        juce::NormalisableRange<float> (0.0f, 18.0f, 0.1f), 2.0f));
    p.add (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "pitch", 1 }, "Pitch",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f));
    p.add (std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { "tone", 1 }, "Tone",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.7f));
    p.add (std::make_unique<juce::AudioParameterChoice>(juce::ParameterID { "mode", 1 }, "Mode",
        juce::StringArray { "CLASSIC", "DRIP", "DREAM" }, 0));
    return p;
}

bool FuturoSpringAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();
    if (input != output)
        return false;
    return input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo();
}

void FuturoSpringAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    currentSampleRate = juce::jmax (8000.0, sampleRate);
    const auto fs = static_cast<float> (currentSampleRate);

    for (int ch = 0; ch < maxChannels; ++ch)
    {
        for (int i = 0; i < numDiffusers; ++i)
        {
            const float stereoOffset = ch == 0 ? 0.0f : 0.00031f * static_cast<float> (i + 1);
            diffusers[static_cast<size_t> (ch * numDiffusers + i)].prepare (
                juce::jmax (2, juce::roundToInt ((diffuserTimesSeconds[static_cast<size_t> (i)] + stereoOffset) * fs)));
        }

        for (int i = 0; i < numResonators; ++i)
        {
            const float stereoOffset = ch == 0 ? 0.0f : 0.00117f * static_cast<float> (i + 1);
            resonators[static_cast<size_t> (ch * numResonators + i)].prepare (
                juce::jmax (2, juce::roundToInt ((resonatorTimesSeconds[static_cast<size_t> (i)] + stereoOffset) * fs)));
            resonatorDamping[static_cast<size_t> (ch * numResonators + i)] = 0.0f;
        }
    }

    dampingState.fill (0.0f);
    previousInput.fill (0.0f);

    minimumPitchDelaySamples = fs * 0.020f;
    pitchSweepRangeSamples = fs * 0.090f;
    pitchBufferSize = static_cast<size_t> (juce::jmax (512, juce::roundToInt (fs * 0.140f) + 8));
    for (auto& channel : pitchBuffers)
        channel.assign (pitchBufferSize, 0.0f);
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
    for (auto& line : diffusers)
        line.clear();
    for (auto& line : resonators)
        line.clear();
    for (auto& channel : pitchBuffers)
        std::fill (channel.begin(), channel.end(), 0.0f);
}

float FuturoSpringAudioProcessor::processDiffuser (int channel, float input, float coefficient) noexcept
{
    float x = input;
    for (int i = 0; i < numDiffusers; ++i)
    {
        auto& line = diffusers[static_cast<size_t> (channel * numDiffusers + i)];
        const float delayed = line.read();
        const float y = delayed - coefficient * x;
        line.write (x + coefficient * y);
        x = y;
    }
    return x;
}

float FuturoSpringAudioProcessor::readPitchDelay (int channel, float delaySamples) const noexcept
{
    if (pitchBufferSize < 4)
        return 0.0f;

    delaySamples = juce::jlimit (1.0f, static_cast<float> (pitchBufferSize - 2), delaySamples);
    const float readPosition = static_cast<float> (pitchWritePosition) - delaySamples;
    const auto index0 = wrapIndex (static_cast<int> (std::floor (readPosition)), static_cast<int> (pitchBufferSize));
    const auto index1 = (index0 + 1) % static_cast<int> (pitchBufferSize);
    const float fraction = readPosition - std::floor (readPosition);
    const auto& data = pitchBuffers[static_cast<size_t> (channel)];
    return data[static_cast<size_t> (index0)] + fraction * (data[static_cast<size_t> (index1)] - data[static_cast<size_t> (index0)]);
}

float FuturoSpringAudioProcessor::saturate (float input, float amount) const noexcept
{
    const float drive = juce::jlimit (0.0f, 1.0f, amount / 18.0f);
    if (drive < 0.0005f)
        return input;

    const float gain = 1.0f + 5.0f * drive;
    const float norm = std::tanh (gain);
    return norm > 0.0001f ? std::tanh (input * gain) / norm : input;
}

void FuturoSpringAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ignoreUnused (midi);
    juce::ScopedNoDenormals noDenormals;

    const int outputChannels = juce::jmin (maxChannels, getTotalNumOutputChannels());
    const int inputChannels = juce::jmin (maxChannels, getTotalNumInputChannels());
    const int numSamples = buffer.getNumSamples();
    if (inputChannels < 1 || outputChannels < 1 || pitchBufferSize < 4)
        return;

    for (int ch = inputChannels; ch < outputChannels; ++ch)
        buffer.clear (ch, 0, numSamples);

    const float targetMix = parameters.getRawParameterValue ("mix")->load();
    const float targetIntensity = parameters.getRawParameterValue ("intensity")->load();
    const float targetSaturation = parameters.getRawParameterValue ("saturation")->load();
    const float targetPitch = parameters.getRawParameterValue ("pitch")->load();
    const float targetTone = parameters.getRawParameterValue ("tone")->load();
    const float targetMode = parameters.getRawParameterValue ("mode")->load();

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
        const float modeValue = juce::jlimit (0.0f, 2.0f, smoothMode);

        const float classicWeight = modeValue < 1.0f ? 1.0f - modeValue : 0.0f;
        const float dripWeight = modeValue < 1.0f ? modeValue : 2.0f - modeValue;
        const float dreamWeight = modeValue > 1.0f ? modeValue - 1.0f : 0.0f;
        const float feedback = (0.585f + 0.335f * smoothIntensity) * classicWeight
                             + (0.535f + 0.315f * smoothIntensity) * dripWeight
                             + (0.645f + 0.305f * smoothIntensity) * dreamWeight;
        const float diffusion = 0.57f * classicWeight + 0.48f * dripWeight + 0.68f * dreamWeight;
        const float excitation = 0.22f * classicWeight + 0.34f * dripWeight + 0.17f * dreamWeight;
        const float toneCutoff = juce::jlimit (900.0f, 0.46f * static_cast<float> (currentSampleRate),
                                                1200.0f * std::pow (14.0f, smoothTone));
        const float toneCoefficient = 1.0f - std::exp (-twoPi * toneCutoff / static_cast<float> (currentSampleRate));

        float wet[2] { 0.0f, 0.0f };
        const float inputs[2] { dryL, dryR };
        for (int ch = 0; ch < inputChannels; ++ch)
        {
            const int other = inputChannels > 1 ? 1 - ch : ch;
            const float crossInput = inputs[ch] * 0.92f + inputs[other] * 0.08f;
            const float shapedInput = saturate (crossInput, smoothSaturation);
            const float diffused = processDiffuser (ch, shapedInput, diffusion);

            float resonatorSum = 0.0f;
            for (int i = 0; i < numResonators; ++i)
            {
                const size_t index = static_cast<size_t> (ch * numResonators + i);
                auto& line = resonators[index];
                const float delayed = line.read();
                resonatorDamping[index] += toneCoefficient * (delayed - resonatorDamping[index]);
                const float highPart = delayed - resonatorDamping[index];
                const float brightWeight = 0.16f * dripWeight - 0.08f * dreamWeight;
                const float feedbackSignal = resonatorDamping[index] + highPart * (smoothTone * 0.35f + brightWeight);
                const float loopInput = diffused * excitation + feedbackSignal * feedback;
                line.write (juce::jlimit (-2.0f, 2.0f, saturate (loopInput, smoothSaturation * 0.65f)));
                resonatorSum += delayed * resonatorGains[static_cast<size_t> (i)];
            }

            const float bodyGain = 1.0f + 1.30f * smoothIntensity;
            const float modeWetGain = 1.00f * classicWeight + 1.28f * dripWeight + 0.88f * dreamWeight;
            wet[ch] = saturate (resonatorSum * bodyGain * modeWetGain, smoothSaturation * 0.65f);
        }

        const float pitchEnabled = std::abs (smoothPitch) > 0.025f ? 1.0f : 0.0f;
        pitchWetBlend += parameterSmoothing * (pitchEnabled - pitchWetBlend);
        const float ratio = std::pow (2.0f, smoothPitch / 12.0f);
        const float phaseStep = std::abs (1.0f - ratio) / juce::jmax (1.0f, pitchSweepRangeSamples);

        for (int ch = 0; ch < inputChannels; ++ch)
            pitchBuffers[static_cast<size_t> (ch)][pitchWritePosition] = wet[ch];

        float shifted[2] { wet[0], wet[1] };
        if (pitchWetBlend > 0.0001f && std::abs (smoothPitch) > 0.025f)
        {
            const bool pitchUp = ratio > 1.0f;
            float phaseA = pitchPhase;
            float phaseB = pitchPhase + 0.5f;
            if (phaseB >= 1.0f)
                phaseB -= 1.0f;
            const auto delayForPhase = [this, pitchUp] (float phase)
            {
                const float ramp = pitchUp ? (1.0f - phase) : phase;
                return minimumPitchDelaySamples + ramp * pitchSweepRangeSamples;
            };
            const float weightA = 0.5f - 0.5f * std::cos (twoPi * phaseA);
            const float weightB = 1.0f - weightA;
            for (int ch = 0; ch < inputChannels; ++ch)
            {
                const float a = readPitchDelay (ch, delayForPhase (phaseA));
                const float b = readPitchDelay (ch, delayForPhase (phaseB));
                shifted[ch] = a * weightA + b * weightB;
            }
            pitchPhase += phaseStep;
            if (pitchPhase >= 1.0f)
                pitchPhase -= std::floor (pitchPhase);
        }

        const float mix = juce::jlimit (0.0f, 1.0f, smoothMix);
        const float wetL = wet[0] * (1.0f - pitchWetBlend) + shifted[0] * pitchWetBlend;
        const float wetR = inputChannels > 1 ? wet[1] * (1.0f - pitchWetBlend) + shifted[1] * pitchWetBlend : wetL;
        buffer.setSample (0, n, dryL * (1.0f - mix) + wetL * mix);
        if (outputChannels > 1)
            buffer.setSample (1, n, dryR * (1.0f - mix) + wetR * mix);

        if (++pitchWritePosition >= pitchBufferSize)
            pitchWritePosition = 0;
    }
}

juce::AudioProcessorEditor* FuturoSpringAudioProcessor::createEditor()
{
    return new FuturoSpringAudioProcessorEditor (*this);
}

void FuturoSpringAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, dest);
}

void FuturoSpringAudioProcessor::setStateInformation (const void* data, int size)
{
    if (data == nullptr || size <= 0)
        return;
    if (auto xml = getXmlFromBinary (data, size))
    {
        if (xml->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xml));
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FuturoSpringAudioProcessor();
}
