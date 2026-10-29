#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class FuturoKnobLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool isMouseOverButton, bool isButtonDown) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&,
                         bool isMouseOverButton, bool isButtonDown) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox&) override;
    void drawLabel (juce::Graphics&, juce::Label&) override;
};

class FuturoSpringAudioProcessorEditor : public juce::AudioProcessorEditor,
                                         private juce::Timer
{
public:
    explicit FuturoSpringAudioProcessorEditor (FuturoSpringAudioProcessor&);
    ~FuturoSpringAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    FuturoSpringAudioProcessor& processor;
    FuturoKnobLookAndFeel knobLookAndFeel;
    juce::Slider mix, intensity, saturation, character, tone, inputGain, outputGain;
    juce::TextButton classic { "CLASSICUS" }, flex { "NOVUS" }, dream { "SOMNIUM" }, fiuso { "FIUSO" };
    juce::Label heading, subheading, inputGainLabel, outputGainLabel, presetLabel, moodCaption;
    juce::ComboBox presetMenu;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SliderAttachment> mixAtt, intensityAtt, saturationAtt, characterAtt, toneAtt, inputGainAtt, outputGainAtt;
    std::unique_ptr<ComboAttachment> modeAtt;
    std::unique_ptr<ButtonAttachment> fiusoAtt;
    juce::ComboBox modeParam;
    float animPhase = 0.0f;
    float animatedIntensity = 0.60f;
    int visualMode = 0;

    void styleKnob (juce::Slider&, const juce::String&);
    void styleFader (juce::Slider&, const juce::String&);
    void timerCallback() override;
    void setMode (int);
    void applyPreset (int presetId);
    juce::Colour currentModeColour() const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FuturoSpringAudioProcessorEditor)
};
