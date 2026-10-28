#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

class FuturoSpringAudioProcessor : public juce::AudioProcessor
{
public:
    FuturoSpringAudioProcessor();
    ~FuturoSpringAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "FUTURO Spring Reverb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 12.0; }
    bool supportsDoublePrecisionProcessing() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    using APVTS = juce::AudioProcessorValueTreeState;
    APVTS parameters;
    static APVTS::ParameterLayout createParameterLayout();

private:
    struct DelayLine
    {
        std::vector<float> data;
        size_t writeIndex = 0;

        void prepare (int delaySamples);
        void clear();
        float read() const noexcept;
        void write (float sample) noexcept;
    };

    static constexpr int numDiffusers = 4;
    static constexpr int numResonators = 4;
    static constexpr int numModes = 3;
    static constexpr int maxChannels = 2;
    static constexpr float twoPi = 6.2831853071795864769f;

    std::array<DelayLine, maxChannels * numModes * numDiffusers> diffusers;
    std::array<DelayLine, maxChannels * numModes * numResonators> resonators;
    std::array<float, maxChannels * numModes * numResonators> resonatorDamping {};
    std::array<float, maxChannels> dampingState {};
    std::array<float, maxChannels> wetPresenceLowpassState {};
    std::array<float, maxChannels> previousInput {};

    std::array<float, maxChannels> characterLowpassState {};
    std::array<float, maxChannels> classicusToneState {};
    float voltageSagEnvelope = 0.0f;
    float voltageSagGain = 1.0f;

    // Linked stereo lookahead limiter preserves headroom without intentional saturation.
    std::array<std::vector<float>, maxChannels> outputDelayBuffers;
    size_t outputDelayWritePosition = 0;
    size_t outputDelaySamples = 0;
    float outputPeakEnvelope = 0.0f;
    float outputPeakRelease = 0.999f;

    double currentSampleRate = 44100.0;
    float smoothMix = 0.25f;
    float smoothIntensity = 0.5f;
    float smoothSaturation = 2.0f;
    float smoothCharacter = 0.35f;
    float smoothTone = 0.7f;
    float smoothMode = 0.0f;
    float parameterSmoothing = 0.002f;

    float processDiffuser (int channel, int mode, float input, float coefficient) noexcept;
    float saturate (float input, float amount) const noexcept;
    float saturateVintage (float input, float amount) const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FuturoSpringAudioProcessor)
};
