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
    static constexpr int maxChannels = 2;
    static constexpr float twoPi = 6.2831853071795864769f;

    std::array<DelayLine, maxChannels * numDiffusers> diffusers;
    std::array<DelayLine, maxChannels * numResonators> resonators;
    std::array<float, maxChannels * numResonators> resonatorDamping {};
    std::array<float, maxChannels> dampingState {};
    std::array<float, maxChannels> previousInput {};

    std::array<std::vector<float>, maxChannels> pitchBuffers;
    size_t pitchWritePosition = 0;
    size_t pitchBufferSize = 0;
    float pitchPhase = 0.0f;
    float pitchWetBlend = 0.0f;
    float minimumPitchDelaySamples = 0.0f;
    float pitchSweepRangeSamples = 0.0f;

    double currentSampleRate = 44100.0;
    float smoothMix = 0.25f;
    float smoothIntensity = 0.5f;
    float smoothSaturation = 2.0f;
    float smoothPitch = 0.0f;
    float smoothTone = 0.7f;
    float smoothMode = 0.0f;
    float parameterSmoothing = 0.002f;

    float processDiffuser (int channel, float input, float coefficient) noexcept;
    float readPitchDelay (int channel, float delaySamples) const noexcept;
    float saturate (float input, float amount) const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FuturoSpringAudioProcessor)
};
