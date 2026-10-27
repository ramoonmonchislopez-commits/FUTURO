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
    juce::Slider mix, intensity, saturation, pitch, tone;
    juce::TextButton classic { "CLASSIC" }, flex { "FLEX" }, dream { "DREAM" };
    juce::Label heading, subheading;
    juce::Label mixCaption, driveCaption, decayCaption, pitchCaption, toneCaption;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    std::unique_ptr<SliderAttachment> mixAtt, intensityAtt, saturationAtt, pitchAtt, toneAtt;
    std::unique_ptr<ComboAttachment> modeAtt;
    juce::ComboBox modeParam;
    float animPhase = 0.0f;
    int visualMode = 0;

    void styleKnob (juce::Slider&, const juce::String&);
    void timerCallback() override;
    void setMode (int);
    juce::Colour currentModeColour() const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FuturoSpringAudioProcessorEditor)
};
