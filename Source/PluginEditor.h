#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class FuturoKnobLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;
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
    juce::TextButton classic { "CLASSICUS" }, flex { "NOVUS" }, dream { "SOMNIUM" };
    juce::TextButton fiusoButton { "Fiuso" };
    juce::Label heading, subheading, inputLabel, outputLabel;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SliderAttachment> mixAtt, intensityAtt, saturationAtt, characterAtt, toneAtt,
                                      inputAtt, outputAtt;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> fiusoAtt;
    std::unique_ptr<ComboAttachment> modeAtt;
    juce::ComboBox modeParam;
    float animPhase = 0.0f;
    int visualMode = 0;

    void styleKnob (juce::Slider&, const juce::String&);
    void styleMiniFader (juce::Slider&, const juce::String&, juce::Label&);
    void timerCallback() override;
    void setMode (int);
    juce::Colour currentModeColour() const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FuturoSpringAudioProcessorEditor)
};
