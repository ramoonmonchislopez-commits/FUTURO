#include "PluginEditor.h"

#include <cmath>

namespace
{
    const juce::Colour paper    { 0xff151a25 };
    const juce::Colour ink      { 0xfff4ecdf };
    const juce::Colour orange   { 0xffffb08f };
    const juce::Colour teal     { 0xff67d4cc };
    const juce::Colour blue     { 0xff70c3ff };
    const juce::Colour violet   { 0xffce9bff };
    const juce::Colour cream    { 0xfffbf3e6 };
    const juce::Colour line     { 0xff4c586a };
    const juce::Colour green    { 0xff83d779 };
    const juce::Colour rose     { 0xffffb8a8 };
    const juce::Colour charcoal { 0xff343743 };
    const juce::Colour surface  { 0xff242b39 };
    const juce::Colour faceInk  { 0xff18212d };
    const juce::Colour redGlow  { 0xffff776e };

    juce::Colour blendModeColour (int mode)
    {
        return mode == 0 ? orange : (mode == 1 ? teal : violet);
    }
}

void FuturoKnobLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                               float sliderPosProportional, float rotaryStartAngle,
                                               float rotaryEndAngle, juce::Slider& slider)
{
    const float labelHeight = 17.0f;
    const float dialHeight = juce::jmax (1.0f, static_cast<float> (height) - labelHeight);
    const float side = static_cast<float> (juce::jmin (width - 4, juce::roundToInt (dialHeight)));
    const float radius = side * 0.375f;
    const float centreX = static_cast<float> (x) + static_cast<float> (width) * 0.5f;
    const float centreY = static_cast<float> (y) + labelHeight + dialHeight * 0.5f;
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);

    g.setColour (ink.withAlpha (0.88f));
    g.setFont (juce::Font (juce::FontOptions (8.8f, juce::Font::bold)));
    g.drawText (slider.getName().toUpperCase(), x + 1, y, width - 2,
                static_cast<int> (labelHeight), juce::Justification::centred);

    // Gunmetal dial, metallic edge and restrained neon halo.
    g.setColour (accent.withAlpha (0.11f));
    g.drawEllipse (centreX - radius - 7.0f, centreY - radius - 7.0f,
                   radius * 2.0f + 14.0f, radius * 2.0f + 14.0f, 4.0f);
    g.setColour (surface.brighter (0.18f));
    g.fillEllipse (centreX - radius - 2.0f, centreY - radius - 2.0f,
                   radius * 2.0f + 4.0f, radius * 2.0f + 4.0f);

    juce::ColourGradient faceGradient (juce::Colour (0xff3a4557),
                                        centreX - radius, centreY - radius,
                                        juce::Colour (0xff171d28),
                                        centreX + radius, centreY + radius, false);
    g.setGradientFill (faceGradient);
    g.fillEllipse (centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);
    g.setColour (cream.withAlpha (0.18f));
    g.drawEllipse (centreX - radius + 3.0f, centreY - radius + 3.0f,
                   radius * 2.0f - 6.0f, radius * 2.0f - 6.0f, 1.0f);

    juce::Path trackArc;
    trackArc.addCentredArc (centreX, centreY, radius + 4.8f, radius + 4.8f, 0.0f,
                            rotaryStartAngle - juce::MathConstants<float>::halfPi,
                            rotaryEndAngle - juce::MathConstants<float>::halfPi, true);
    g.setColour (line.withAlpha (0.74f));
    g.strokePath (trackArc, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));

    juce::Path activeArc;
    activeArc.addCentredArc (centreX, centreY, radius + 4.8f, radius + 4.8f, 0.0f,
                             rotaryStartAngle - juce::MathConstants<float>::halfPi,
                             angle - juce::MathConstants<float>::halfPi, true);
    g.setColour (accent.withAlpha (0.20f));
    g.strokePath (activeArc, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (activeArc, juce::PathStrokeType (2.8f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

    const float pointerAngle = angle - juce::MathConstants<float>::halfPi;
    g.setColour (cream);
    g.drawLine (centreX + std::cos (pointerAngle) * radius * 0.15f,
                centreY + std::sin (pointerAngle) * radius * 0.15f,
                centreX + std::cos (pointerAngle) * radius * 0.68f,
                centreY + std::sin (pointerAngle) * radius * 0.68f, 2.0f);
    g.setColour (accent);
    g.fillEllipse (centreX - 2.0f, centreY - 2.0f, 4.0f, 4.0f);
}

FuturoSpringAudioProcessorEditor::FuturoSpringAudioProcessorEditor (FuturoSpringAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (960, 640);
    setResizable (true, true);
    setResizeLimits (860, 620, 1240, 840);
    setLookAndFeel (&knobLookAndFeel);

    heading.setText ("FUTURO", juce::dontSendNotification);
    heading.setFont (juce::Font (juce::FontOptions (38.0f)));
    heading.setColour (juce::Label::textColourId, cream);
    heading.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (heading);

    subheading.setText ("REVERB   ·   BY RAMON LOPEZ", juce::dontSendNotification);
    subheading.setFont (juce::Font (juce::FontOptions (10.5f)));
    subheading.setColour (juce::Label::textColourId, cream.withAlpha (0.68f));
    subheading.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (subheading);

    presetMenu.addItem ("Copper Bloom", 1);
    presetMenu.addItem ("Studio Coil", 2);
    presetMenu.addItem ("Voltage Kiss", 3);
    presetMenu.addItem ("Lunar Halo", 4);
    presetMenu.addItem ("Temple Bloom", 5);
    presetMenu.setTextWhenNothingSelected ("COPPER BLOOM");
    presetMenu.setTooltip ("Five contrasted factory voices: Copper Bloom, Studio Coil, Voltage Kiss, Lunar Halo, and Temple Bloom. All controls remain editable.");
    presetMenu.setColour (juce::ComboBox::backgroundColourId, surface);
    presetMenu.setColour (juce::ComboBox::textColourId, cream);
    presetMenu.setColour (juce::ComboBox::outlineColourId, line);
    presetMenu.setColour (juce::ComboBox::arrowColourId, orange);
    presetMenu.onChange = [this]
    {
        const int selectedPreset = presetMenu.getSelectedId();
        if (selectedPreset > 0)
        {
            const auto selectedName = presetMenu.getText();
            applyPreset (selectedPreset);
            presetMenu.setTextWhenNothingSelected (selectedName);
        }
        presetMenu.setSelectedId (0, juce::dontSendNotification);
    };
    addAndMakeVisible (presetMenu);

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
        label->setColour (juce::Label::textColourId, cream.withAlpha (0.82f));
        label->setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (*label);
    }
    inputGainLabel.setText ("IN", juce::dontSendNotification);
    outputGainLabel.setText ("OUT", juce::dontSendNotification);

    classic.setButtonText ("✦  CLASSICUS");
    flex.setButtonText ("≋  NOVUS");
    dream.setButtonText ("☾  SOMNIUM");

    for (auto* button : { &classic, &flex, &dream })
    {
        button->setClickingTogglesState (true);
        button->setRadioGroupId (901);
        button->setColour (juce::TextButton::buttonColourId, surface.withAlpha (0.90f));
        button->setColour (juce::TextButton::buttonOnColourId, orange);
        button->setColour (juce::TextButton::textColourOffId, cream.withAlpha (0.78f));
        button->setColour (juce::TextButton::textColourOnId, paper);
        button->setConnectedEdges (0);
        addAndMakeVisible (*button);
    }

    classic.onClick = [this] { setMode (0); };
    flex.onClick    = [this] { setMode (1); };
    dream.onClick   = [this] { setMode (2); };

    fiuso.setClickingTogglesState (true);
    fiuso.setColour (juce::TextButton::buttonColourId, surface);
    fiuso.setColour (juce::TextButton::buttonOnColourId, orange);
    fiuso.setColour (juce::TextButton::textColourOffId, cream.withAlpha (0.82f));
    fiuso.setColour (juce::TextButton::textColourOnId, paper);
    fiuso.setButtonText ("≋  FIŪSO  /  HARMONICS");
    fiuso.setTooltip ("Fiuso adds octave-up and related even-harmonic texture to the spring excitation; the direct path stays clean");
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
    animatedIntensity = juce::jlimit (0.0f, 1.0f,
        p.parameters.getRawParameterValue ("intensity")->load());
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
    slider.setColour (juce::Slider::trackColourId, line.brighter (0.15f));
    slider.setColour (juce::Slider::thumbColourId, currentModeColour());
    slider.setColour (juce::Slider::textBoxTextColourId, cream);
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
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, line.withAlpha (0.9f));
    slider.setColour (juce::Slider::textBoxTextColourId, cream);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, paper.withAlpha (0.0f));
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxHighlightColourId, orange.withAlpha (0.38f));
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
    for (auto* slider : { &inputGain, &outputGain })
        slider->setColour (juce::Slider::thumbColourId, accent);
    for (auto* button : { &classic, &flex, &dream })
        button->setColour (juce::TextButton::buttonOnColourId, accent);
    repaint();
}


void FuturoSpringAudioProcessorEditor::applyPreset (int presetId)
{
    // Factory presets are starting points, not locked modes. Every value is sent
    // through the host parameter API so automation and project-state recall work.
    const auto setParameter = [this] (const juce::String& parameterID, float value)
    {
        if (auto* parameter = processor.parameters.getParameter (parameterID))
        {
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
            parameter->endChangeGesture();
        }
    };

    switch (presetId)
    {
        case 1: // Copper Bloom — the default Classicus reference voice.
            setParameter ("mode", 0.0f);
            setParameter ("mix", 0.31f);
            setParameter ("intensity", 0.60f);
            setParameter ("saturation", 3.8f);
            setParameter ("character", 62.0f);
            setParameter ("tone", 0.54f);
            setParameter ("inputGain", 0.0f);
            setParameter ("outputGain", -0.5f);
            setParameter ("fiuso", 0.0f);
            break;

        case 2: // Studio Coil — short, clean spring that sits behind the source.
            setParameter ("mode", 0.0f);
            setParameter ("mix", 0.17f);
            setParameter ("intensity", 0.27f);
            setParameter ("saturation", 1.1f);
            setParameter ("character", 22.0f);
            setParameter ("tone", 0.82f);
            setParameter ("inputGain", 0.0f);
            setParameter ("outputGain", 0.0f);
            setParameter ("fiuso", 0.0f);
            break;

        case 3: // Voltage Kiss — pronounced sag, upper-mid bite and lively return.
            setParameter ("mode", 1.0f);
            setParameter ("mix", 0.36f);
            setParameter ("intensity", 0.66f);
            setParameter ("saturation", 5.3f);
            setParameter ("character", 80.0f);
            setParameter ("tone", 0.74f);
            setParameter ("inputGain", -1.5f);
            setParameter ("outputGain", -1.0f);
            setParameter ("fiuso", 0.0f);
            break;

        case 4: // Lunar Halo — dark, spacious and deliberately low on Drive.
            setParameter ("mode", 2.0f);
            setParameter ("mix", 0.39f);
            setParameter ("intensity", 0.70f);
            setParameter ("saturation", 2.4f);
            setParameter ("character", 56.0f);
            setParameter ("tone", 0.22f);
            setParameter ("inputGain", -1.5f);
            setParameter ("outputGain", -1.0f);
            setParameter ("fiuso", 0.0f);
            break;

        case 5: // Temple Bloom — harmonic shimmer with Fiūso, but controlled Drive.
            setParameter ("mode", 2.0f);
            setParameter ("mix", 0.48f);
            setParameter ("intensity", 0.76f);
            setParameter ("saturation", 4.2f);
            setParameter ("character", 72.0f);
            setParameter ("tone", 0.44f);
            setParameter ("inputGain", -2.0f);
            setParameter ("outputGain", -2.0f);
            setParameter ("fiuso", 1.0f);
            break;

        default:
            break;
    }
}

void FuturoSpringAudioProcessorEditor::timerCallback()
{
    const float targetIntensity = juce::jlimit (0.0f, 1.0f,
        processor.parameters.getRawParameterValue ("intensity")->load());
    const float intensityDelta = targetIntensity - animatedIntensity;

    // Follow the host parameter with an eased UI value; face and wings never snap.
    animatedIntensity += intensityDelta * 0.12f;
    if (std::abs (intensityDelta) < 0.001f)
        animatedIntensity = targetIntensity;

    animPhase += 0.035f + animatedIntensity * 0.09f;
    if (animPhase > juce::MathConstants<float>::twoPi)
        animPhase -= juce::MathConstants<float>::twoPi;

    const int hostMode = juce::jlimit (0, 2,
        static_cast<int> (processor.parameters.getRawParameterValue ("mode")->load()));
    if (hostMode != visualMode)
        setMode (hostMode);

    repaint();
}

void FuturoSpringAudioProcessorEditor::paint (juce::Graphics& g)
{
    const int width = getWidth();
    const int height = getHeight();
    const auto outer = getLocalBounds().toFloat().reduced (10.0f);
    const auto modeColour = currentModeColour();
    const float mood = juce::jlimit (0.0f, 1.0f, animatedIntensity);
    const auto smoothStep = [] (float value)
    {
        const float t = juce::jlimit (0.0f, 1.0f, value);
        return t * t * (3.0f - 2.0f * t);
    };
    const float happy = smoothStep (mood);
    const float sad = 1.0f - smoothStep (mood / 0.44f);
    const float devil = smoothStep ((mood - 0.56f) / 0.44f);
    const float pulse = std::sin (animPhase * (1.15f + mood * 0.95f));
    const float breathe = std::sin (animPhase * 1.7f);
    const auto auraColour = modeColour.interpolatedWith (orange, devil * 0.78f);

    // Retro-futurist glass panel, deep ink and a pair of warm/cool edge glows.
    juce::ColourGradient backgroundGradient (juce::Colour (0xff0d111a), 0.0f, 0.0f,
                                              juce::Colour (0xff222a3a),
                                              static_cast<float> (width), static_cast<float> (height), false);
    g.setGradientFill (backgroundGradient);
    g.fillAll();

    juce::ColourGradient panelGradient (juce::Colour (0xf32c3445), outer.getX(), outer.getY(),
                                         juce::Colour (0xf41a202d), outer.getRight(), outer.getBottom(), false);
    g.setGradientFill (panelGradient);
    g.fillRoundedRectangle (outer, 20.0f);

    g.setColour (blue.withAlpha (0.14f));
    g.drawRoundedRectangle (outer.reduced (1.2f), 20.0f, 5.0f);
    g.setColour (orange.withAlpha (0.35f));
    g.drawRoundedRectangle (outer, 20.0f, 1.1f);
    g.setColour (line.withAlpha (0.65f));
    g.drawRoundedRectangle (outer.reduced (5.0f), 16.0f, 0.8f);

    // Sparse scanlines and technical guide marks keep the finish analog, not noisy.
    g.setColour (cream.withAlpha (0.018f));
    for (int y = 112; y < height - 188; y += 5)
        g.drawLine (24.0f, static_cast<float> (y), static_cast<float> (width - 24),
                    static_cast<float> (y), 0.45f);

    g.setColour (line.withAlpha (0.68f));
    g.drawLine (30.0f, 103.0f, static_cast<float> (width - 30), 103.0f, 0.8f);
    g.setColour (auraColour);
    g.fillEllipse (width - 48.0f, 31.0f, 7.0f, 7.0f);
    g.setColour (cream.withAlpha (0.68f));
    g.setFont (juce::Font (juce::FontOptions (9.2f)));
    g.drawText ("SPRING REVERB  /  ANALOG IMAGINATION", width - 360, 45, 300, 17,
                juce::Justification::centredRight);

    // Technical target marks around the characters.
    const float cx = width * 0.595f;
    const float cy = 246.0f;
    const float ringBreath = 1.0f + 0.025f * breathe * (0.25f + mood);
    const float ringW = (216.0f + mood * 38.0f) * ringBreath;
    const float ringH = (262.0f + mood * 43.0f) * ringBreath;

    for (int i = 3; i >= 0; --i)
    {
        const float expansion = 1.0f + static_cast<float> (i) * 0.13f + devil * 0.08f;
        const float alpha = 0.075f + mood * 0.04f + devil * 0.075f - static_cast<float> (i) * 0.012f;
        g.setColour (auraColour.withAlpha (alpha));
        g.drawEllipse (cx - ringW * expansion * 0.5f, cy - ringH * expansion * 0.5f,
                       ringW * expansion, ringH * expansion, i == 0 ? 1.35f : 0.75f);
    }

    // Two slow orbit rings and a few warm particles react to the character's mood.
    const float orbitRx = ringW * 0.54f;
    const float orbitRy = ringH * 0.53f;
    for (int i = 0; i < 9; ++i)
    {
        const float a = animPhase * (0.45f + devil * 0.50f)
                      + juce::MathConstants<float>::twoPi * static_cast<float> (i) / 9.0f;
        const float dot = 1.3f + mood * 1.2f + devil * 1.6f;
        const auto dotColour = (i % 3 == 0 ? orange : modeColour)
                               .withAlpha (0.24f + mood * 0.22f + devil * 0.24f);
        g.setColour (dotColour);
        g.fillEllipse (cx + std::cos (a) * orbitRx - dot * 0.5f,
                       cy + std::sin (a) * orbitRy - dot * 0.5f, dot, dot);
    }

    // Draw the angel pair upright, retaining the green embrace, blue body and
    // their pink/charcoal wings from the supplied drawing.
    const float characterScale = 0.79f + mood * 0.13f
                               + breathe * (0.003f + mood * 0.011f);
    const float bob = pulse * (1.0f + mood * 4.2f);
    const float wingOpen = 0.83f + mood * 0.24f + std::sin (animPhase * 1.3f) * 0.014f;
    g.saveState();
    g.addTransform (juce::AffineTransform (characterScale, 0.0f, cx,
                                           0.0f, characterScale, cy + bob));

    // Devil tail is an accent that fades in only near the top of Intensity.
    juce::Path tail;
    tail.startNewSubPath (-43.0f, 74.0f);
    tail.cubicTo (-22.0f, 82.0f, -4.0f, 77.0f, -8.0f, 63.0f);
    tail.cubicTo (-13.0f, 52.0f, 2.0f, 55.0f, 5.0f, 66.0f);
    g.setColour (redGlow.withAlpha (devil * 0.95f));
    g.strokePath (tail, juce::PathStrokeType (3.1f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    juce::Path tailSpade;
    tailSpade.startNewSubPath (4.0f, 64.0f);
    tailSpade.lineTo (13.0f, 66.0f);
    tailSpade.lineTo (5.0f, 74.0f);
    tailSpade.lineTo (-2.0f, 67.0f);
    tailSpade.closeSubPath();
    g.setColour (orange.withAlpha (devil * 0.95f));
    g.fillPath (tailSpade);

    // Peach wing opens as the angels become bolder.
    g.saveState();
    g.addTransform (juce::AffineTransform (wingOpen, 0.0f, -42.0f * (wingOpen - 1.0f),
                                           0.0f, 1.0f, 0.0f));
    juce::Path peachWing;
    peachWing.startNewSubPath (-42.0f, -5.0f);
    peachWing.cubicTo (-64.0f, -22.0f, -91.0f, -10.0f, -91.0f, 11.0f);
    peachWing.cubicTo (-91.0f, 31.0f, -73.0f, 45.0f, -55.0f, 37.0f);
    peachWing.cubicTo (-48.0f, 34.0f, -43.0f, 25.0f, -39.0f, 17.0f);
    peachWing.closeSubPath();
    juce::Path peachFeathers;
    peachFeathers.startNewSubPath (-48.0f, -1.0f);
    peachFeathers.cubicTo (-67.0f, -17.0f, -80.0f, -8.0f, -78.0f, 5.0f);
    peachFeathers.startNewSubPath (-47.0f, 8.0f);
    peachFeathers.cubicTo (-68.0f, -1.0f, -78.0f, 12.0f, -65.0f, 23.0f);
    peachFeathers.startNewSubPath (-45.0f, 16.0f);
    peachFeathers.cubicTo (-58.0f, 10.0f, -67.0f, 27.0f, -54.0f, 31.0f);
    g.setColour (rose.withAlpha (0.96f));
    g.fillPath (peachWing);
    g.setColour (cream.withAlpha (0.94f));
    g.strokePath (peachFeathers, juce::PathStrokeType (3.8f, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
    g.restoreState();

    // Tall blue figure, positioned behind the green angel.
    juce::Path blueBody;
    blueBody.startNewSubPath (10.0f, -55.0f);
    blueBody.cubicTo (-2.0f, -62.0f, -4.0f, -76.0f, 5.0f, -88.0f);
    blueBody.cubicTo (15.0f, -102.0f, 34.0f, -95.0f, 37.0f, -79.0f);
    blueBody.cubicTo (39.0f, -68.0f, 35.0f, -59.0f, 29.0f, -52.0f);
    blueBody.cubicTo (31.0f, -18.0f, 42.0f, 48.0f, 58.0f, 104.0f);
    blueBody.cubicTo (63.0f, 120.0f, 53.0f, 132.0f, 41.0f, 124.0f);
    blueBody.cubicTo (24.0f, 111.0f, 19.0f, 60.0f, 12.0f, 17.0f);
    blueBody.cubicTo (9.0f, 1.0f, 4.0f, -4.0f, -2.0f, 8.0f);
    blueBody.cubicTo (-10.0f, 22.0f, -22.0f, 14.0f, -16.0f, 2.0f);
    blueBody.cubicTo (-8.0f, -15.0f, 2.0f, -36.0f, 10.0f, -55.0f);
    blueBody.closeSubPath();
    g.setColour (blue);
    g.fillPath (blueBody);

    // Charcoal feathers animate open with the same eased parameter.
    g.saveState();
    g.addTransform (juce::AffineTransform (wingOpen, 0.0f, 31.0f * (1.0f - wingOpen),
                                           0.0f, 1.0f, 0.0f));
    juce::Path darkWing;
    darkWing.startNewSubPath (29.0f, -33.0f);
    darkWing.cubicTo (44.0f, -57.0f, 70.0f, -57.0f, 87.0f, -42.0f);
    darkWing.cubicTo (105.0f, -27.0f, 110.0f, -9.0f, 95.0f, 1.0f);
    darkWing.cubicTo (83.0f, 10.0f, 68.0f, -2.0f, 57.0f, -7.0f);
    darkWing.cubicTo (46.0f, -10.0f, 37.0f, -6.0f, 31.0f, 2.0f);
    darkWing.cubicTo (27.0f, -8.0f, 24.0f, -22.0f, 29.0f, -33.0f);
    darkWing.closeSubPath();
    g.setColour (charcoal);
    g.fillPath (darkWing);
    juce::Path darkFeathers;
    for (int i = 0; i < 3; ++i)
    {
        const float yy = -39.0f + static_cast<float> (i) * 11.0f;
        darkFeathers.startNewSubPath (37.0f, yy);
        darkFeathers.cubicTo (54.0f, yy - 9.0f, 77.0f, yy + 2.0f, 91.0f, yy + 13.0f);
    }
    g.setColour (cream.withAlpha (0.57f));
    g.strokePath (darkFeathers, juce::PathStrokeType (1.9f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
    g.setColour (cream.withAlpha (0.68f));
    g.drawEllipse (30.0f, -30.0f, 13.0f, 13.0f, 2.0f);
    g.restoreState();

    // Expression interpolation: tearful frown at minimum, grin and horns at maximum.
    // Eyes remain tiny black dots to preserve the look of the hand-drawn characters.
    g.setColour (faceInk);
    g.fillEllipse (16.0f, -80.0f, 4.7f, 4.7f);
    g.setColour (cream.withAlpha (0.75f));
    g.fillEllipse (17.0f, -79.0f, 1.15f, 1.15f);

    juce::Path blueMouth;
    blueMouth.startNewSubPath (12.0f, -68.0f);
    blueMouth.cubicTo (14.0f, -75.0f + mood * 13.0f,
                       20.0f, -75.0f + mood * 13.0f, 22.0f, -68.0f);
    g.setColour (faceInk);
    g.strokePath (blueMouth, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));

    juce::Path blueBrow;
    blueBrow.startNewSubPath (12.0f, -86.0f);
    blueBrow.lineTo (21.0f, -85.0f + devil * 3.0f);
    g.setColour (faceInk.withAlpha (0.84f));
    g.strokePath (blueBrow, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));

    // A tear slides gently down the blue angel's cheek while Intensity is low.
    juce::Path blueTear;
    blueTear.startNewSubPath (18.0f, -71.0f + (1.0f - sad) * 4.0f);
    blueTear.cubicTo (16.0f, -67.0f, 17.0f, -64.0f, 19.0f, -64.0f);
    blueTear.cubicTo (22.0f, -64.0f, 22.0f, -68.0f, 18.0f, -71.0f + (1.0f - sad) * 4.0f);
    blueTear.closeSubPath();
    g.setColour (blue.brighter (0.23f).withAlpha (sad * 0.92f));
    g.fillPath (blueTear);

    // Green angel in front, reaching around its companion.
    juce::Path greenBody;
    greenBody.startNewSubPath (-42.0f, -13.0f);
    greenBody.cubicTo (-55.0f, -20.0f, -56.0f, -38.0f, -47.0f, -48.0f);
    greenBody.cubicTo (-38.0f, -59.0f, -23.0f, -53.0f, -21.0f, -40.0f);
    greenBody.cubicTo (-19.0f, -29.0f, -24.0f, -20.0f, -30.0f, -13.0f);
    greenBody.cubicTo (-28.0f, 14.0f, -23.0f, 53.0f, -25.0f, 88.0f);
    greenBody.cubicTo (-26.0f, 105.0f, -34.0f, 119.0f, -43.0f, 116.0f);
    greenBody.cubicTo (-54.0f, 113.0f, -50.0f, 97.0f, -47.0f, 79.0f);
    greenBody.cubicTo (-43.0f, 48.0f, -49.0f, 23.0f, -49.0f, 7.0f);
    greenBody.cubicTo (-49.0f, -2.0f, -47.0f, -8.0f, -42.0f, -13.0f);
    greenBody.closeSubPath();
    g.setColour (green);
    g.fillPath (greenBody);

    juce::Path embraceArm;
    embraceArm.startNewSubPath (-37.0f, 4.0f);
    embraceArm.cubicTo (-24.0f, -1.0f, -13.0f, -18.0f, -2.0f, -31.0f);
    embraceArm.cubicTo (8.0f, -43.0f, 17.0f, -48.0f, 23.0f, -41.0f);
    embraceArm.cubicTo (28.0f, -35.0f, 17.0f, -27.0f, 9.0f, -19.0f);
    embraceArm.cubicTo (-2.0f, -7.0f, -15.0f, 8.0f, -29.0f, 17.0f);
    embraceArm.cubicTo (-36.0f, 21.0f, -42.0f, 15.0f, -37.0f, 4.0f);
    embraceArm.closeSubPath();
    g.fillPath (embraceArm);

    juce::Path greenHighlight;
    greenHighlight.startNewSubPath (-44.0f, -35.0f);
    greenHighlight.cubicTo (-48.0f, -23.0f, -44.0f, -12.0f, -41.0f, -2.0f);
    g.setColour (cream.withAlpha (0.30f));
    g.strokePath (greenHighlight, juce::PathStrokeType (1.1f, juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));

    g.setColour (faceInk);
    g.fillEllipse (-39.0f, -39.0f, 4.2f, 4.2f);
    g.setColour (cream.withAlpha (0.72f));
    g.fillEllipse (-38.0f, -38.0f, 1.0f, 1.0f);

    juce::Path greenMouth;
    greenMouth.startNewSubPath (-44.0f, -27.0f);
    greenMouth.cubicTo (-42.0f, -34.0f + mood * 12.0f,
                        -36.0f, -34.0f + mood * 12.0f, -34.0f, -27.0f);
    g.setColour (faceInk);
    g.strokePath (greenMouth, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

    juce::Path greenBrow;
    greenBrow.startNewSubPath (-44.0f, -45.0f);
    greenBrow.lineTo (-35.0f, -44.0f + devil * 3.0f);
    g.setColour (faceInk.withAlpha (0.84f));
    g.strokePath (greenBrow, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

    juce::Path greenTear;
    greenTear.startNewSubPath (-36.0f, -29.0f + (1.0f - sad) * 3.0f);
    greenTear.cubicTo (-39.0f, -25.0f, -37.0f, -22.0f, -35.0f, -23.0f);
    greenTear.cubicTo (-33.0f, -24.0f, -33.0f, -27.0f, -36.0f, -29.0f + (1.0f - sad) * 3.0f);
    greenTear.closeSubPath();
    g.setColour (blue.brighter (0.23f).withAlpha (sad * 0.90f));
    g.fillPath (greenTear);

    // The original halo slowly gives way to little devil horns near full Intensity.
    juce::Path halo;
    halo.addEllipse (-50.0f, -64.0f, 27.0f, 7.3f);
    g.setColour (blue.withAlpha (0.90f * (1.0f - devil)));
    g.strokePath (halo, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    g.setColour (cream.withAlpha (0.92f * (1.0f - devil)));
    g.drawLine (-45.0f, -60.3f, -28.0f, -60.3f, 0.8f);

    juce::Path blueHorns;
    blueHorns.startNewSubPath (7.0f, -88.0f);
    blueHorns.lineTo (1.0f, -103.0f);
    blueHorns.lineTo (15.0f, -91.0f);
    blueHorns.closeSubPath();
    blueHorns.startNewSubPath (26.0f, -88.0f);
    blueHorns.lineTo (36.0f, -103.0f);
    blueHorns.lineTo (35.0f, -86.0f);
    blueHorns.closeSubPath();

    juce::Path greenHorns;
    greenHorns.startNewSubPath (-46.0f, -48.0f);
    greenHorns.lineTo (-54.0f, -61.0f);
    greenHorns.lineTo (-41.0f, -51.0f);
    greenHorns.closeSubPath();
    greenHorns.startNewSubPath (-31.0f, -49.0f);
    greenHorns.lineTo (-23.0f, -61.0f);
    greenHorns.lineTo (-27.0f, -46.0f);
    greenHorns.closeSubPath();

    g.setColour (redGlow.withAlpha (devil * 0.22f));
    g.strokePath (blueHorns, juce::PathStrokeType (6.0f));
    g.strokePath (greenHorns, juce::PathStrokeType (6.0f));
    g.setColour (orange.withAlpha (devil));
    g.fillPath (blueHorns);
    g.fillPath (greenHorns);
    g.setColour (cream.withAlpha (devil * 0.82f));
    g.drawLine (4.0f, -99.0f, 7.0f, -94.0f, 0.8f);
    g.drawLine (32.0f, -99.0f, 32.0f, -92.0f, 0.8f);

    // Keep each mode's signature accessory visible on top of the animation.
    if (visualMode == 0)
    {
        g.setColour (orange.withAlpha (0.94f));
        g.drawLine (57.0f, -96.0f, 57.0f, -84.0f, 1.8f);
        g.drawLine (51.0f, -90.0f, 63.0f, -90.0f, 1.8f);
        g.drawLine (53.0f, -94.0f, 61.0f, -86.0f, 1.0f);
        g.drawLine (61.0f, -94.0f, 53.0f, -86.0f, 1.0f);
    }
    else if (visualMode == 1)
    {
        juce::Path bolt;
        bolt.startNewSubPath (57.0f, -107.0f);
        bolt.lineTo (47.0f, -92.0f);
        bolt.lineTo (55.0f, -93.0f);
        bolt.lineTo (51.0f, -80.0f);
        bolt.lineTo (66.0f, -99.0f);
        bolt.lineTo (58.0f, -97.0f);
        bolt.closeSubPath();
        g.setColour (orange);
        g.fillPath (bolt);
        g.setColour (cream.withAlpha (0.85f));
        g.drawLine (57.0f, -101.0f, 53.0f, -94.0f, 1.0f);
    }
    else
    {
        g.setColour (violet.withAlpha (0.95f));
        g.fillEllipse (50.0f, -106.0f, 16.0f, 16.0f);
        g.setColour (cream);
        g.fillEllipse (57.0f, -111.0f, 16.0f, 16.0f);
        g.setColour (violet);
        g.fillEllipse (73.0f, -93.0f, 3.6f, 3.6f);
        g.fillEllipse (47.0f, -87.0f, 2.6f, 2.6f);
    }

    if (fiuso.getToggleState())
    {
        juce::Path jewel;
        jewel.startNewSubPath (48.0f, 115.0f);
        jewel.lineTo (55.0f, 125.0f);
        jewel.lineTo (48.0f, 135.0f);
        jewel.lineTo (41.0f, 125.0f);
        jewel.closeSubPath();
        g.setColour (orange.withAlpha (0.85f));
        g.fillPath (jewel);
        g.setColour (cream);
        g.drawLine (48.0f, 118.0f, 48.0f, 132.0f, 0.9f);
    }
    g.restoreState();

    // Mode and mood readouts sit under the artwork without competing with it.
    const juce::String modeName = visualMode == 0 ? "CLASSICUS"
                                : visualMode == 1 ? "NOVUS" : "SOMNIUM";
    const juce::String modeSubtitle = visualMode == 0 ? "SPRING  /  WARMTH"
                                   : visualMode == 1 ? "SAG  /  VOLTAGE"
                                   : "HALO  /  DREAM";
    g.setColour (cream.withAlpha (0.90f));
    g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    g.drawText (modeName, juce::roundToInt (cx - 120.0f), 389, 240, 17,
                juce::Justification::centred);
    g.setColour (auraColour.withAlpha (0.80f));
    g.setFont (juce::Font (juce::FontOptions (8.6f)));
    g.drawText (modeSubtitle, juce::roundToInt (cx - 120.0f), 407, 240, 14,
                juce::Justification::centred);

    g.setColour (line.withAlpha (0.72f));
    g.drawLine (30.0f, static_cast<float> (height - 195), static_cast<float> (width - 30),
                static_cast<float> (height - 195), 0.8f);
    g.setColour (auraColour.withAlpha (0.85f));
    g.fillEllipse (width - 47.0f, height - 31.0f, 6.0f, 6.0f);
    g.setColour (cream.withAlpha (0.52f));
    g.setFont (juce::Font (juce::FontOptions (8.3f)));
    g.drawText ("DIFFERENT ANGELS. SAME DREAM.", 30, height - 30, 260, 14,
                juce::Justification::centredLeft);
}

void FuturoSpringAudioProcessorEditor::resized()
{
    const int width = getWidth();
    const int height = getHeight();
    heading.setBounds (36, 22, 300, 46);
    subheading.setBounds (40, 67, 420, 20);
    presetMenu.setBounds (width - 292, 66, 260, 27);

    const int left = 34;
    classic.setBounds (left, 151, 142, 35);
    flex.setBounds (left, 196, 142, 35);
    dream.setBounds (left, 241, 142, 35);
    fiuso.setBounds (left, 286, 142, 32);

    inputGainLabel.setBounds (left + 2, 344, 42, 14);
    inputGain.setBounds (left, 358, 142, 24);
    outputGainLabel.setBounds (left + 2, 390, 42, 14);
    outputGain.setBounds (left, 404, 142, 24);

    const float fractions[5] { 0.16f, 0.34f, 0.54f, 0.74f, 0.89f };
    const int knobTop = height - 180;
    const int standardWidth = 104;
    const int standardHeight = 126;
    const int largeWidth = 146;
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
