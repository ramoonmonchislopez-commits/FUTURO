#include "PluginEditor.h"

#include <cmath>

namespace
{
    const juce::Colour paper  { 0xffd8c49a };
    const juce::Colour ink    { 0xff211d18 };
    const juce::Colour orange { 0xffc65c28 };
    const juce::Colour teal   { 0xff247d78 };
    const juce::Colour blue   { 0xff426d91 };
    const juce::Colour violet { 0xff786a9f };
    const juce::Colour cream  { 0xfff0e2bd };

    juce::Colour blendModeColour (int mode)
    {
        return mode == 0 ? orange : (mode == 1 ? teal : violet);
    }
}

void FuturoKnobLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                               float sliderPosProportional, float rotaryStartAngle,
                                               float rotaryEndAngle, juce::Slider& slider)
{
    // Reserve a dedicated label band above the dial so long, mode-dependent names
    // never collide with the rotary artwork or the value box below it.
    const float labelHeight = 17.0f;
    const float dialHeight = juce::jmax (1.0f, static_cast<float> (height) - labelHeight);
    const float side = static_cast<float> (juce::jmin (width, juce::roundToInt (dialHeight)));
    const float radius = side * 0.405f;
    const float centreX = static_cast<float> (x) + static_cast<float> (width) * 0.5f;
    const float centreY = static_cast<float> (y) + labelHeight + dialHeight * 0.5f;
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
    const auto outline = slider.findColour (juce::Slider::rotarySliderOutlineColourId);

    g.setColour (ink.withAlpha (0.88f));
    g.setFont (juce::Font (juce::FontOptions (8.5f, juce::Font::bold)));
    g.drawText (slider.getName(), x + 2, y, width - 4, static_cast<int> (labelHeight), juce::Justification::centred);

    g.setColour (ink.withAlpha (0.16f));
    g.fillEllipse (centreX - radius + 2.5f, centreY - radius + 3.0f, radius * 2.0f, radius * 2.0f);
    g.setColour (outline);
    g.fillEllipse (centreX - radius - 2.0f, centreY - radius - 2.0f, radius * 2.0f + 4.0f, radius * 2.0f + 4.0f);
    g.setColour (paper.brighter (0.06f));
    g.fillEllipse (centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

    for (int i = 0; i <= 16; ++i)
    {
        const float proportion = static_cast<float> (i) / 16.0f;
        const float tickAngle = rotaryStartAngle + proportion * (rotaryEndAngle - rotaryStartAngle)
                              - juce::MathConstants<float>::halfPi;
        const float inner = radius * (i % 4 == 0 ? 0.76f : 0.83f);
        const float outer = radius * 0.91f;
        g.setColour (i / 16.0f <= sliderPosProportional ? accent : ink.withAlpha (0.40f));
        g.drawLine (centreX + inner * std::cos (tickAngle), centreY + inner * std::sin (tickAngle),
                    centreX + outer * std::cos (tickAngle), centreY + outer * std::sin (tickAngle),
                    i % 4 == 0 ? 1.8f : 1.0f);
    }

    g.setColour (ink.withAlpha (0.90f));
    g.fillEllipse (centreX - radius * 0.68f, centreY - radius * 0.68f,
                   radius * 1.36f, radius * 1.36f);
    g.setColour (paper.withAlpha (0.18f));
    g.drawEllipse (centreX - radius * 0.59f, centreY - radius * 0.59f,
                   radius * 1.18f, radius * 1.18f, 1.0f);

    const float pointerAngle = angle - juce::MathConstants<float>::halfPi;
    const float pointerInner = radius * 0.15f;
    const float pointerOuter = radius * 0.61f;
    g.setColour (accent);
    g.drawLine (centreX + pointerInner * std::cos (pointerAngle),
                centreY + pointerInner * std::sin (pointerAngle),
                centreX + pointerOuter * std::cos (pointerAngle),
                centreY + pointerOuter * std::sin (pointerAngle), 3.0f);
    g.fillEllipse (centreX - 3.0f, centreY - 3.0f, 6.0f, 6.0f);
}

FuturoSpringAudioProcessorEditor::FuturoSpringAudioProcessorEditor (FuturoSpringAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (960, 640);
    setResizable (true, true);
    setResizeLimits (860, 620, 1240, 840);
    setLookAndFeel (&knobLookAndFeel);

    heading.setText ("SPIRITUS", juce::dontSendNotification);
    heading.setFont (juce::Font (juce::FontOptions (41.0f, juce::Font::bold)));
    heading.setColour (juce::Label::textColourId, ink);
    heading.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (heading);

    subheading.setText ("SPRING REVERB   /   THREE DISTINCT CHARACTERS", juce::dontSendNotification);
    subheading.setFont (juce::Font (juce::FontOptions (11.0f)));
    subheading.setColour (juce::Label::textColourId, ink.withAlpha (0.78f));
    subheading.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (subheading);

    styleKnob (mix, "MIX");
    styleKnob (saturation, "DRIVE");
    saturation.setTooltip ("Mode-specific drive: vintage warmth, power-stage compression, or soft rounded saturation");
    styleKnob (intensity, "INTENSITY");
    intensity.setTooltip ("More reverb body and a longer spring decay; the upper range pushes harder");
    styleKnob (character, "SPRING");
    character.setTextValueSuffix (" %");
    styleKnob (tone, "TONE");
    styleFader (inputGain, "IN");
    styleFader (outputGain, "OUT");
    for (auto* label : { &inputGainLabel, &outputGainLabel })
    {
        label->setFont (juce::Font (juce::FontOptions (9.0f, juce::Font::bold)));
        label->setColour (juce::Label::textColourId, ink.withAlpha (0.86f));
        label->setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (*label);
    }
    inputGainLabel.setText ("IN", juce::dontSendNotification);
    outputGainLabel.setText ("OUT", juce::dontSendNotification);

    classic.setButtonText ("CLASSICUS");
    flex.setButtonText ("NOVUS");
    dream.setButtonText ("SOMNIUM");

    for (auto* button : { &classic, &flex, &dream })
    {
        button->setClickingTogglesState (true);
        button->setRadioGroupId (901);
        button->setColour (juce::TextButton::buttonColourId, paper.darker (0.08f));
        button->setColour (juce::TextButton::buttonOnColourId, orange);
        button->setColour (juce::TextButton::textColourOffId, ink);
        button->setColour (juce::TextButton::textColourOnId, cream);
        button->setConnectedEdges (juce::Button::ConnectedOnLeft | juce::Button::ConnectedOnRight);
        addAndMakeVisible (*button);
    }

    classic.onClick = [this] { setMode (0); };
    flex.onClick    = [this] { setMode (1); };
    dream.onClick   = [this] { setMode (2); };

    fiuso.setClickingTogglesState (true);
    fiuso.setColour (juce::TextButton::buttonColourId, ink);
    fiuso.setColour (juce::TextButton::buttonOnColourId, orange);
    fiuso.setColour (juce::TextButton::textColourOffId, cream);
    fiuso.setColour (juce::TextButton::textColourOnId, cream);
    fiuso.setTooltip ("FIUSO adds an octave-up harmonic layer to the spring excitation; the direct path stays clean");
    addAndMakeVisible (fiuso);

    modeParam.addItem ("Classicus", 1);
    modeParam.addItem ("Novus", 2);
    modeParam.addItem ("Somnium", 3);
    modeAtt = std::make_unique<ComboAttachment> (p.parameters, "mode", modeParam);
    modeParam.setVisible (false);

    mixAtt        = std::make_unique<SliderAttachment> (p.parameters, "mix", mix);
    intensityAtt  = std::make_unique<SliderAttachment> (p.parameters, "intensity", intensity);
    saturationAtt = std::make_unique<SliderAttachment> (p.parameters, "saturation", saturation);
    characterAtt      = std::make_unique<SliderAttachment> (p.parameters, "character", character);
    toneAtt       = std::make_unique<SliderAttachment> (p.parameters, "tone", tone);
    inputGainAtt  = std::make_unique<SliderAttachment> (p.parameters, "inputGain", inputGain);
    outputGainAtt = std::make_unique<SliderAttachment> (p.parameters, "outputGain", outputGain);
    fiusoAtt      = std::make_unique<ButtonAttachment> (p.parameters, "fiuso", fiuso);

    setMode (static_cast<int> (p.parameters.getRawParameterValue ("mode")->load()));
    startTimerHz (24);
}

FuturoSpringAudioProcessorEditor::~FuturoSpringAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
    for (auto* slider : { &mix, &intensity, &saturation, &character, &tone, &inputGain, &outputGain })
        slider->setLookAndFeel (nullptr);
}

void FuturoSpringAudioProcessorEditor::styleFader (juce::Slider& slider, const juce::String& label)
{
    slider.setName (label);
    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 48, 18);
    slider.setTextValueSuffix (" dB");
    slider.setLookAndFeel (&knobLookAndFeel);
    slider.setColour (juce::Slider::trackColourId, teal);
    slider.setColour (juce::Slider::thumbColourId, ink);
    slider.setColour (juce::Slider::textBoxTextColourId, ink);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, paper.withAlpha (0.0f));
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (slider);
}

void FuturoSpringAudioProcessorEditor::styleKnob (juce::Slider& slider, const juce::String& label)
{
    slider.setName (label);
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 82, 18);
    slider.setLookAndFeel (&knobLookAndFeel);
    slider.setColour (juce::Slider::rotarySliderFillColourId, orange);
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, ink.withAlpha (0.78f));
    slider.setColour (juce::Slider::textBoxTextColourId, ink);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, paper.withAlpha (0.0f));
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxHighlightColourId, orange.withAlpha (0.35f));
    addAndMakeVisible (slider);
}

juce::Colour FuturoSpringAudioProcessorEditor::currentModeColour() const noexcept
{
    return blendModeColour (visualMode);
}

void FuturoSpringAudioProcessorEditor::setMode (int mode)
{
    visualMode = juce::jlimit (0, 2, mode);
    modeParam.setSelectedId (visualMode + 1, juce::sendNotificationSync);
    classic.setToggleState (visualMode == 0, juce::dontSendNotification);
    flex.setToggleState (visualMode == 1, juce::dontSendNotification);
    dream.setToggleState (visualMode == 2, juce::dontSendNotification);

    character.setName (visualMode == 0 ? "Spring" : visualMode == 1 ? "Sag" : "Halo");
    character.setTooltip (visualMode == 0
        ? "CLASSICUS: Spring — vintage spring colour and controlled harmonic drive; no pitch modulation"
        : visualMode == 1
            ? "NOVUS: Sag — Voltage Sag, a controlled level dip with a slow recovery on the reverb return"
            : "SOMNIUM: Halo — a dark, soft halo on the reverb return; fixed-time colour, no pitch modulation");

    const auto accent = currentModeColour();
    for (auto* slider : { &mix, &intensity, &saturation, &character, &tone })
        slider->setColour (juce::Slider::rotarySliderFillColourId, accent);
    fiuso.setColour (juce::TextButton::buttonOnColourId, accent);
    for (auto* button : { &classic, &flex, &dream })
        button->setColour (juce::TextButton::buttonOnColourId, accent);
    repaint();
}

void FuturoSpringAudioProcessorEditor::timerCallback()
{
    animPhase += 0.035f + processor.parameters.getRawParameterValue ("intensity")->load() * 0.09f;
    if (animPhase > juce::MathConstants<float>::twoPi)
        animPhase -= juce::MathConstants<float>::twoPi;

    const int hostMode = juce::jlimit (0, 2,
        static_cast<int> (processor.parameters.getRawParameterValue ("mode")->load()));
    if (hostMode != visualMode)
        setMode (hostMode);
    else
        repaint();
}

void FuturoSpringAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (paper);
    const auto outer = getLocalBounds().toFloat().reduced (12.0f);
    const auto modeColour = currentModeColour();

    g.setColour (ink);
    g.drawRoundedRectangle (outer, 13.0f, 2.0f);
    g.setColour (ink.withAlpha (0.24f));
    g.drawRoundedRectangle (outer.reduced (7.0f), 10.0f, 1.0f);

    g.setColour (ink.withAlpha (0.055f));
    for (int i = 0; i < 180; ++i)
    {
        const int gx = 23 + ((i * 97 + 31) % juce::jmax (1, getWidth() - 46));
        const int gy = 18 + ((i * 53 + 17) % juce::jmax (1, getHeight() - 36));
        const float diameter = (i % 7 == 0) ? 1.8f : 0.8f;
        g.fillEllipse (static_cast<float> (gx), static_cast<float> (gy), diameter, diameter);
    }

    g.setColour (ink.withAlpha (0.34f));
    g.drawLine (30.0f, 105.0f, static_cast<float> (getWidth() - 30), 105.0f, 1.0f);
    g.setColour (modeColour);
    g.fillEllipse (getWidth() - 48.0f, 31.0f, 9.0f, 9.0f);
    g.setColour (ink.withAlpha (0.78f));
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    g.drawText ("ANALOG IMAGINATION / DIGITAL RESONANCE", getWidth() - 350, 48, 300, 18,
                juce::Justification::centredRight);

    const float intensityValue = processor.parameters.getRawParameterValue ("intensity")->load();
    const float cx = getWidth() * 0.56f;
    const float cy = 245.0f;
    const float radius = 91.0f + intensityValue * 26.0f;
    const float breathing = 1.0f + 0.035f * std::sin (animPhase * 1.8f) * intensityValue;

    for (int i = 0; i < 4; ++i)
    {
        const float ringScale = (1.0f + i * 0.105f) * breathing;
        const auto orbit = juce::Rectangle<float> (cx - radius * ringScale,
            cy - radius * 0.73f * ringScale, radius * 2.0f * ringScale, radius * 1.46f * ringScale);
        g.setColour (modeColour.withAlpha (0.38f - static_cast<float> (i) * 0.055f));
        g.drawEllipse (orbit, i == 0 ? 1.8f : 1.1f);
    }
    for (int i = 0; i < 11; ++i)
    {
        const float angle = animPhase + juce::MathConstants<float>::twoPi * static_cast<float> (i) / 11.0f;
        const float orbitX = radius * (1.0f + 0.08f * std::sin (animPhase * 1.4f));
        const float orbitY = radius * 0.73f;
        const float px = cx + std::cos (angle) * orbitX;
        const float py = cy + std::sin (angle) * orbitY;
        const float dotSize = 2.0f + intensityValue * 2.3f;
        g.setColour (modeColour.withAlpha (0.60f));
        g.fillEllipse (px - dotSize * 0.5f, py - dotSize * 0.5f, dotSize, dotSize);
    }

    const float bob = std::sin (animPhase * 1.8f) * (1.5f + intensityValue * 7.0f);
    const float faceSize = 92.0f + intensityValue * 8.0f;
    g.setColour (ink);
    g.fillRoundedRectangle (cx - 33.0f, cy + 29.0f + bob, 66.0f, 70.0f, 17.0f);
    g.setColour (modeColour);
    g.fillEllipse (cx - faceSize * 0.5f - 5.0f, cy - faceSize * 0.5f + bob,
                   faceSize + 10.0f, faceSize + 10.0f);
    g.setColour (cream);
    g.fillEllipse (cx - faceSize * 0.5f, cy - faceSize * 0.5f + 5.0f + bob, faceSize, faceSize);
    g.setColour (ink.withAlpha (0.28f));
    g.drawEllipse (cx - faceSize * 0.5f + 4.0f, cy - faceSize * 0.5f + 9.0f + bob,
                   faceSize - 8.0f, faceSize - 8.0f, 1.0f);

    const float eyeHeight = 5.0f + intensityValue * 8.5f;
    const float eyeY = cy - 12.0f + bob - intensityValue * 2.0f;
    g.setColour (ink);
    g.fillEllipse (cx - 21.0f, eyeY - eyeHeight * 0.5f, 11.0f, eyeHeight);
    g.fillEllipse (cx + 10.0f, eyeY - eyeHeight * 0.5f, 11.0f, eyeHeight);
    g.drawLine (cx - 24.0f, cy - 27.0f + bob, cx - 8.0f, cy - 24.0f + bob, 2.0f);
    g.drawLine (cx + 8.0f, cy - 24.0f + bob, cx + 24.0f, cy - 27.0f + bob, 2.0f);

    const float mouthWidth = 13.0f + intensityValue * 20.0f;
    const float mouthHeight = 7.0f + intensityValue * 12.0f;
    g.setColour (ink);
    if (intensityValue < 0.15f)
        g.drawLine (cx - 7.0f, cy + 11.0f + bob, cx + 7.0f, cy + 11.0f + bob, 2.0f);
    else
    {
        juce::Path smile;
        smile.addArc (cx - mouthWidth * 0.5f, cy + 3.0f + bob, mouthWidth, mouthHeight,
                      0.20f, juce::MathConstants<float>::pi - 0.20f, true);
        g.strokePath (smile, juce::PathStrokeType (2.3f + intensityValue));
    }

    if (visualMode == 0)
    {
        g.setColour (ink);
        g.fillEllipse (cx - 43.0f, cy - 48.0f + bob, 86.0f, 12.0f);
        g.fillRoundedRectangle (cx - 25.0f, cy - 64.0f + bob, 50.0f, 21.0f, 4.0f);
        g.setColour (cream);
        g.drawLine (cx - 13.0f, cy - 57.0f + bob, cx + 13.0f, cy - 57.0f + bob, 1.0f);
    }
    else if (visualMode == 1)
    {
        juce::Path prongs;
        prongs.startNewSubPath (cx - 33.0f, cy - 35.0f + bob);
        prongs.lineTo (cx - 53.0f, cy - 69.0f - intensityValue * 6.0f + bob);
        prongs.lineTo (cx - 20.0f, cy - 49.0f + bob);
        prongs.lineTo (cx + 28.0f, cy - 36.0f + bob);
        prongs.lineTo (cx + 54.0f, cy - 67.0f - intensityValue * 6.0f + bob);
        prongs.lineTo (cx + 38.0f, cy - 29.0f + bob);
        prongs.closeSubPath();
        g.setColour (ink);
        g.fillPath (prongs);
        g.setColour (cream.withAlpha (0.75f));
        g.drawLine (cx - 43.0f, cy - 58.0f + bob, cx - 32.0f, cy - 48.0f + bob, 1.2f);
        g.drawLine (cx + 43.0f, cy - 57.0f + bob, cx + 32.0f, cy - 47.0f + bob, 1.2f);
    }
    else
    {
        g.setColour (violet);
        g.drawEllipse (cx - 68.0f, cy - 75.0f + bob, 136.0f, 136.0f, 4.0f);
        g.setColour (blue);
        g.fillEllipse (cx + 42.0f, cy - 71.0f + bob, 16.0f, 16.0f);
        g.setColour (cream);
        g.fillEllipse (cx + 46.0f, cy - 67.0f + bob, 8.0f, 8.0f);
    }

    g.setColour (ink);
    g.setFont (juce::Font (juce::FontOptions (10.5f, juce::Font::bold)));
    g.drawText ("SPIRITUS  /  " + juce::String (visualMode == 0 ? "CLASSICUS" : visualMode == 1 ? "NOVUS" : "SOMNIUM"),
                0, 366, getWidth(), 18, juce::Justification::centred);

    g.setColour (ink.withAlpha (0.34f));
    g.drawLine (30.0f, static_cast<float> (getHeight() - 204), static_cast<float> (getWidth() - 30),
                static_cast<float> (getHeight() - 204), 1.0f);
    g.setColour (modeColour.withAlpha (0.9f));
    g.fillEllipse (getWidth() - 48.0f, getHeight() - 32.0f, 8.0f, 8.0f);

    g.setColour (ink.withAlpha (0.62f));
    g.setFont (juce::Font (juce::FontOptions (8.5f)));
    g.drawText ("SPIRITUS AUDIO  /  MONO + STEREO", 30, getHeight() - 31, 260, 14,
                juce::Justification::centredLeft);
}

void FuturoSpringAudioProcessorEditor::resized()
{
    const int width = getWidth();
    const int height = getHeight();
    heading.setBounds (36, 22, 300, 48);
    subheading.setBounds (40, 69, 420, 20);

    const int left = 34;
    classic.setBounds (left, 139, 116, 35);
    flex.setBounds (left, 183, 116, 35);
    dream.setBounds (left, 227, 116, 35);
    inputGainLabel.setBounds (left, 276, 42, 14);
    inputGain.setBounds (left, 290, 126, 24);
    outputGainLabel.setBounds (left, 314, 42, 14);
    outputGain.setBounds (left, 328, 126, 24);
    fiuso.setBounds (juce::roundToInt (width * 0.56f) - 67, 389, 134, 34);

    const float fractions[5] { 0.25f, 0.40f, 0.55f, 0.70f, 0.85f };
    const int knobTop = height - 180;
    const int standardWidth = 108;
    const int standardHeight = 126;
    const int largeWidth = 144;
    const int largeHeight = 144;

    mix.setBounds (juce::roundToInt (width * fractions[0]) - standardWidth / 2,
                   knobTop + 9, standardWidth, standardHeight);
    saturation.setBounds (juce::roundToInt (width * fractions[1]) - standardWidth / 2,
                          knobTop + 9, standardWidth, standardHeight);
    intensity.setBounds (juce::roundToInt (width * fractions[2]) - largeWidth / 2,
                         knobTop - 1, largeWidth, largeHeight);
    character.setBounds (juce::roundToInt (width * fractions[3]) - standardWidth / 2,
                     knobTop + 9, standardWidth, standardHeight);
    tone.setBounds (juce::roundToInt (width * fractions[4]) - standardWidth / 2,
                    knobTop + 9, standardWidth, standardHeight);
}
