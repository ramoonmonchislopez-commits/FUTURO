#include "PluginEditor.h"

#include <cmath>
#include <cstdint>

namespace
{
    const juce::Colour paper    { 0xfff1e6cb };
    const juce::Colour ink      { 0xff33414f };
    const juce::Colour orange   { 0xffc8693a };
    const juce::Colour teal     { 0xff3f7f8c };
    const juce::Colour blue     { 0xff3b82d6 };
    const juce::Colour violet   { 0xff7b4a73 };
    const juce::Colour cream    { 0xfffff8e6 };
    const juce::Colour line     { 0xff8a7660 };
    const juce::Colour green    { 0xff43a047 };
    const juce::Colour rose     { 0xffff9ca7 };
    const juce::Colour charcoal { 0xff33414f };
    const juce::Colour surface  { 0xffe3d2ac };
    const juce::Colour faceInk  { 0xff26384a };
    const juce::Colour redGlow  { 0xffbd5b4a };
    const juce::Colour demonWing { 0xff7b4a73 };
    const juce::Colour chalkHighlight { 0xfffff2c6 };
    const juce::Colour soft     { 0xff8a7660 };

    juce::Colour blendModeColour (int mode)
    {
        return mode == 0 ? orange : (mode == 1 ? violet : teal);
    }


    // Deterministic pastel/chalk grains: fixed seeds keep the texture from flickering
    // while the characters animate at 24 Hz.
    void drawChalkTexture (juce::Graphics& g, const juce::Path& shape,
                           juce::Colour pigment, std::uint32_t seed,
                           int marks, float strength)
    {
        const auto bounds = shape.getBounds();
        if (bounds.isEmpty())
            return;

        std::uint32_t state = seed;
        const auto nextUnit = [&state]()
        {
            state = state * 1664525u + 1013904223u;
            return static_cast<float> ((state >> 8) & 0x00ffffffu) / 16777215.0f;
        };

        for (int i = 0; i < marks; ++i)
        {
            const float px = bounds.getX() + nextUnit() * bounds.getWidth();
            const float py = bounds.getY() + nextUnit() * bounds.getHeight();
            if (!shape.contains (px, py))
                continue;

            const float grain = nextUnit();
            const float length = 0.45f + nextUnit() * 2.25f;
            const auto tone = (i % 5 == 0) ? chalkHighlight
                              : (i % 3 == 0) ? pigment.brighter (0.25f)
                                             : pigment.darker (0.24f);
            g.setColour (tone.withAlpha (strength * (0.30f + grain * 0.70f)));
            if (i % 4 == 0)
                g.drawLine (px, py, px + length, py + (nextUnit() - 0.5f) * 0.75f,
                            0.35f + nextUnit() * 0.55f);
            else
            {
                const float size = 0.35f + nextUnit() * 0.85f;
                g.fillEllipse (px, py, size, size * (0.55f + nextUnit() * 0.7f));
            }
        }
    }

    void drawChalkBoardWear (juce::Graphics& g, juce::Rectangle<float> bounds)
    {
        juce::Path board;
        board.addRoundedRectangle (bounds, 19.0f);
        drawChalkTexture (g, board, soft, 0x50415052u, 620, 0.055f);

        std::uint32_t state = 0x50415449u;
        const auto nextUnit = [&state]()
        {
            state = state * 1664525u + 1013904223u;
            return static_cast<float> ((state >> 8) & 0x00ffffffu) / 16777215.0f;
        };

        // Very fine, warm paper flecks and dry marks along the bevel.
        for (int i = 0; i < 76; ++i)
        {
            const float side = nextUnit();
            const float along = nextUnit();
            const float len = 1.5f + nextUnit() * 7.0f;
            float x = bounds.getX() + along * bounds.getWidth();
            float y = bounds.getY() + 1.5f + nextUnit() * 8.0f;
            float dx = len, dy = -0.5f + nextUnit();
            if (side < 0.25f)
            {
                y = bounds.getBottom() - 2.0f - nextUnit() * 8.0f;
                dy *= -1.0f;
            }
            else if (side < 0.50f)
            {
                x = bounds.getX() + 1.5f + nextUnit() * 8.0f;
                y = bounds.getY() + along * bounds.getHeight();
                dx = -0.5f + nextUnit();
                dy = len;
            }
            else if (side < 0.75f)
            {
                x = bounds.getRight() - 2.0f - nextUnit() * 8.0f;
                y = bounds.getY() + along * bounds.getHeight();
                dx = 0.5f - nextUnit();
                dy = len;
            }

            g.setColour ((i % 3 == 0 ? chalkHighlight : soft).withAlpha (0.035f + nextUnit() * 0.055f));
            g.drawLine (x, y, x + dx, y + dy, 0.3f + nextUnit() * 0.4f);
        }
    }

    void drawVintageScrew (juce::Graphics& g, float cx, float cy)
    {
        g.setColour (juce::Colour (0xff786144).withAlpha (0.45f));
        g.fillEllipse (cx - 7.2f, cy - 6.3f, 14.4f, 14.4f);
        juce::ColourGradient screwGradient (juce::Colour (0xfff7dfa5), cx - 5.0f, cy - 5.0f,
                                             juce::Colour (0xff8f642e), cx + 5.0f, cy + 5.0f, false);
        g.setGradientFill (screwGradient);
        g.fillEllipse (cx - 6.2f, cy - 6.2f, 12.4f, 12.4f);
        g.setColour (orange.withAlpha (0.95f));
        g.drawEllipse (cx - 6.2f, cy - 6.2f, 12.4f, 12.4f, 0.8f);
        g.setColour (ink.withAlpha (0.88f));
        g.drawLine (cx - 3.1f, cy + 2.4f, cx + 3.0f, cy - 2.4f, 1.1f);
        g.setColour (chalkHighlight.withAlpha (0.9f));
        g.fillEllipse (cx - 1.0f, cy - 1.0f, 2.0f, 2.0f);
    }
}

void FuturoKnobLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                               float sliderPosProportional, float rotaryStartAngle,
                                               float rotaryEndAngle, juce::Slider& slider)
{
    const float labelHeight = 15.0f;
    const float dialHeight = juce::jmax (1.0f, static_cast<float> (height) - labelHeight);
    const float side = static_cast<float> (juce::jmin (width - 4, juce::roundToInt (dialHeight)));
    const float radius = side * 0.435f;
    const float centreX = static_cast<float> (x) + static_cast<float> (width) * 0.5f;
    const float centreY = static_cast<float> (y) + dialHeight * 0.49f;
    const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);

    // The source reference is a warm hand-printed instrument: chalk lettering,
    // pressed brass rims and a paper-toned enamel knob face.
    g.setColour (accent.withAlpha (0.18f));
    g.drawEllipse (centreX - radius - 8.0f, centreY - radius - 8.0f,
                   radius * 2.0f + 16.0f, radius * 2.0f + 16.0f, 1.6f);

    g.setColour (ink.withAlpha (0.18f));
    g.fillEllipse (centreX - radius - 3.0f, centreY - radius - 1.0f,
                   radius * 2.0f + 7.0f, radius * 2.0f + 7.0f);
    juce::ColourGradient brassRim (juce::Colour (0xffffe8ad),
                                    centreX - radius, centreY - radius,
                                    juce::Colour (0xff98662f),
                                    centreX + radius, centreY + radius, false);
    g.setGradientFill (brassRim);
    g.fillEllipse (centreX - radius - 2.2f, centreY - radius - 2.2f,
                   radius * 2.0f + 4.4f, radius * 2.0f + 4.4f);

    juce::ColourGradient enamel (juce::Colour (0xfffff9e9),
                                  centreX - radius * 0.75f, centreY - radius * 0.85f,
                                  juce::Colour (0xffd2bd91),
                                  centreX + radius * 0.55f, centreY + radius * 0.8f, false);
    g.setGradientFill (enamel);
    g.fillEllipse (centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);

    juce::Path face;
    face.addEllipse (centreX - radius, centreY - radius, radius * 2.0f, radius * 2.0f);
    drawChalkTexture (g, face, accent, static_cast<std::uint32_t> (x * 101 + y * 31 + width),
                      70, 0.12f);
    g.setColour (chalkHighlight.withAlpha (0.65f));
    g.drawEllipse (centreX - radius + 3.0f, centreY - radius + 3.0f,
                   radius * 2.0f - 6.0f, radius * 2.0f - 6.0f, 0.8f);

    // Fine engraved tick marks and a coloured chalk arc.
    for (int i = 0; i <= 20; ++i)
    {
        const float p = static_cast<float> (i) / 20.0f;
        const float tickAngle = rotaryStartAngle + p * (rotaryEndAngle - rotaryStartAngle)
                              - juce::MathConstants<float>::halfPi;
        const float inner = radius + 4.0f;
        const float outer = radius + (i % 2 == 0 ? 10.0f : 7.0f);
        g.setColour (i / 20.0f <= sliderPosProportional
                         ? accent.withAlpha (0.92f) : soft.withAlpha (0.55f));
        g.drawLine (centreX + std::cos (tickAngle) * inner,
                    centreY + std::sin (tickAngle) * inner,
                    centreX + std::cos (tickAngle) * outer,
                    centreY + std::sin (tickAngle) * outer,
                    i % 2 == 0 ? 1.0f : 0.7f);
    }

    juce::Path activeArc;
    activeArc.addCentredArc (centreX, centreY, radius + 6.0f, radius + 6.0f, 0.0f,
                             rotaryStartAngle - juce::MathConstants<float>::halfPi,
                             angle - juce::MathConstants<float>::halfPi, true);
    g.setColour (accent.withAlpha (0.16f));
    g.strokePath (activeArc, juce::PathStrokeType (6.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (activeArc, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

    const float pointerAngle = angle - juce::MathConstants<float>::halfPi;
    g.setColour (ink.withAlpha (0.20f));
    g.drawLine (centreX + std::cos (pointerAngle) * radius * 0.12f + 0.8f,
                centreY + std::sin (pointerAngle) * radius * 0.12f + 1.0f,
                centreX + std::cos (pointerAngle) * radius * 0.70f + 0.8f,
                centreY + std::sin (pointerAngle) * radius * 0.70f + 1.0f, 3.0f);
    g.setColour (ink);
    g.drawLine (centreX + std::cos (pointerAngle) * radius * 0.12f,
                centreY + std::sin (pointerAngle) * radius * 0.12f,
                centreX + std::cos (pointerAngle) * radius * 0.70f,
                centreY + std::sin (pointerAngle) * radius * 0.70f, 1.7f);
    g.setColour (accent);
    g.fillEllipse (centreX - 2.1f, centreY - 2.1f, 4.2f, 4.2f);

    g.setColour (ink);
    g.setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::bold)));
    g.drawText (slider.getName().toUpperCase(), x + 1, y + juce::roundToInt (dialHeight),
                width - 2, static_cast<int> (labelHeight), juce::Justification::centred);
}

void FuturoKnobLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                   const juce::Colour& backgroundColour,
                                                   bool isMouseOverButton, bool isButtonDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    juce::Path panel;
    panel.addRoundedRectangle (bounds, 7.0f);
    const auto accent = button.getToggleState()
        ? button.findColour (juce::TextButton::buttonOnColourId) : orange.withAlpha (0.55f);

    juce::ColourGradient gradient (backgroundColour.brighter (isMouseOverButton ? 0.18f : 0.05f),
                                    bounds.getX(), bounds.getY(),
                                    backgroundColour.darker (isButtonDown ? 0.30f : 0.12f),
                                    bounds.getRight(), bounds.getBottom(), false);
    g.setGradientFill (gradient);
    g.fillPath (panel);
    drawChalkTexture (g, panel, accent, static_cast<std::uint32_t> (bounds.getX() * 17.0f
                       + bounds.getY() * 31.0f + bounds.getWidth()), 42, 0.11f);
    g.setColour (accent.withAlpha (button.getToggleState() ? 0.88f : 0.30f));
    g.strokePath (panel, juce::PathStrokeType (button.getToggleState() ? 1.45f : 0.8f));
    if (button.getToggleState())
    {
        g.setColour (accent.withAlpha (0.09f));
        g.fillPath (panel);
    }
}

void FuturoKnobLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                             bool isMouseOverButton, bool isButtonDown)
{
    auto bounds = button.getLocalBounds().reduced (8, 1);
    const auto colour = button.getToggleState()
        ? button.findColour (juce::TextButton::textColourOnId)
        : button.findColour (juce::TextButton::textColourOffId);
    g.setColour (colour.withAlpha (isButtonDown ? 0.72f : 1.0f));
    g.setFont (juce::Font (juce::FontOptions (9.3f, juce::Font::bold)));
    g.drawFittedText (button.getButtonText(), bounds, juce::Justification::centred, 1);
    if (isMouseOverButton)
    {
        g.setColour (button.findColour (juce::TextButton::buttonOnColourId).withAlpha (0.12f));
        g.drawRoundedRectangle (button.getLocalBounds().toFloat().reduced (1.0f), 7.0f, 1.0f);
    }
}

void FuturoKnobLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height,
                                           bool isButtonDown, int buttonX, int buttonY,
                                           int buttonW, int buttonH, juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (width),
                                           static_cast<float> (height)).reduced (1.0f);
    juce::Path panel;
    panel.addRoundedRectangle (bounds, 8.0f);
    const auto bg = box.findColour (juce::ComboBox::backgroundColourId);
    juce::ColourGradient gradient (bg.brighter (0.07f), bounds.getX(), bounds.getY(),
                                    bg.darker (isButtonDown ? 0.28f : 0.10f),
                                    bounds.getRight(), bounds.getBottom(), false);
    g.setGradientFill (gradient);
    g.fillPath (panel);
    drawChalkTexture (g, panel, orange, 0x4355504bu, 58, 0.10f);
    g.setColour (box.findColour (juce::ComboBox::outlineColourId).withAlpha (0.95f));
    g.strokePath (panel, juce::PathStrokeType (1.0f));
    const float arrowCentreX = static_cast<float> (buttonX + buttonW / 2);
    const float arrowCentreY = static_cast<float> (buttonY + buttonH / 2);
    juce::Path arrow;
    arrow.startNewSubPath (arrowCentreX - 4.5f, arrowCentreY - 1.8f);
    arrow.lineTo (arrowCentreX, arrowCentreY + 2.5f);
    arrow.lineTo (arrowCentreX + 4.5f, arrowCentreY - 1.8f);
    g.setColour (box.findColour (juce::ComboBox::arrowColourId));
    g.strokePath (arrow, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));
}

void FuturoKnobLookAndFeel::drawLabel (juce::Graphics& g, juce::Label& label)
{
    const auto bounds = label.getLocalBounds();
    const auto colour = label.findColour (juce::Label::textColourId);
    g.setFont (label.getFont());
    g.setColour (colour.withAlpha (0.16f));
    g.drawFittedText (label.getText(), bounds.translated (0, 1),
                      label.getJustificationType(), 1);
    g.setColour (colour);
    g.drawFittedText (label.getText(), bounds, label.getJustificationType(), 1);
}

FuturoSpringAudioProcessorEditor::FuturoSpringAudioProcessorEditor (FuturoSpringAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (880, 640);
    setResizable (false, false);
    setLookAndFeel (&knobLookAndFeel);

    heading.setText ("SPIRITUS", juce::dontSendNotification);
    heading.setFont (juce::Font (juce::FontOptions (36.0f)));
    heading.setColour (juce::Label::textColourId, ink);
    heading.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (heading);

    subheading.setText ("—  SPRING REVERB  —", juce::dontSendNotification);
    subheading.setFont (juce::Font (juce::FontOptions (10.5f)));
    subheading.setColour (juce::Label::textColourId, soft);
    subheading.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (subheading);

    presetLabel.setText ("PRESET", juce::dontSendNotification);
    presetLabel.setFont (juce::Font (juce::FontOptions (9.5f, juce::Font::bold)));
    presetLabel.setColour (juce::Label::textColourId, soft);
    presetLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (presetLabel);

    moodCaption.setText ("Soñadores", juce::dontSendNotification);
    moodCaption.setFont (juce::Font (juce::FontOptions (12.0f, juce::Font::bold)));
    moodCaption.setColour (juce::Label::textColourId, ink);
    moodCaption.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (moodCaption);

    presetMenu.addItem ("Copper Bloom", 1);
    presetMenu.addItem ("Studio Coil", 2);
    presetMenu.addItem ("Voltage Kiss", 3);
    presetMenu.addItem ("Lunar Halo", 4);
    presetMenu.addItem ("Temple Bloom", 5);
    presetMenu.setTextWhenNothingSelected ("COPPER BLOOM");
    presetMenu.setTooltip ("Five contrasted factory voices: Copper Bloom, Studio Coil, Voltage Kiss, Lunar Halo, and Temple Bloom. All controls remain editable.");
    presetMenu.setColour (juce::ComboBox::backgroundColourId, paper);
    presetMenu.setColour (juce::ComboBox::textColourId, ink);
    presetMenu.setColour (juce::ComboBox::outlineColourId, line.withAlpha (0.85f));
    presetMenu.setColour (juce::ComboBox::arrowColourId, orange);
    presetMenu.setLookAndFeel (&knobLookAndFeel);
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

    mix.setColour (juce::Slider::rotarySliderFillColourId, orange);
    saturation.setColour (juce::Slider::rotarySliderFillColourId, blue);
    intensity.setColour (juce::Slider::rotarySliderFillColourId, currentModeColour());
    character.setColour (juce::Slider::rotarySliderFillColourId, green);
    tone.setColour (juce::Slider::rotarySliderFillColourId, violet);

    mix.setTextFromValueFunction ([] (double value) { return juce::String (juce::roundToInt (value * 100.0)) + "%"; });
    mix.setValueFromTextFunction ([] (const juce::String& text) { return text.getDoubleValue() / 100.0; });
    saturation.setTextFromValueFunction ([] (double value) { return juce::String (juce::roundToInt (value / 18.0 * 100.0)) + "%"; });
    saturation.setValueFromTextFunction ([] (const juce::String& text) { return text.getDoubleValue() * 0.18; });
    intensity.setTextFromValueFunction ([] (double value) { return juce::String (juce::roundToInt (value * 100.0)) + "%"; });
    intensity.setValueFromTextFunction ([] (const juce::String& text) { return text.getDoubleValue() / 100.0; });
    character.setTextFromValueFunction ([] (double value) { return juce::String (juce::roundToInt (value)) + "%"; });
    character.setValueFromTextFunction ([] (const juce::String& text) { return text.getDoubleValue(); });
    tone.setTextFromValueFunction ([] (double value) { return juce::String (juce::roundToInt (value * 100.0)) + "%"; });
    tone.setValueFromTextFunction ([] (const juce::String& text) { return text.getDoubleValue() / 100.0; });
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
    flex.setButtonText ("⚡  NOVUS");
    dream.setButtonText ("☾  SOMNIUM");

    for (auto* button : { &classic, &flex, &dream })
    {
        button->setLookAndFeel (&knobLookAndFeel);
        button->setClickingTogglesState (true);
        button->setRadioGroupId (901);
        button->setColour (juce::TextButton::buttonColourId, paper.withAlpha (0.10f));
        button->setColour (juce::TextButton::buttonOnColourId, orange);
        button->setColour (juce::TextButton::textColourOffId, ink);
        button->setColour (juce::TextButton::textColourOnId, cream);
        button->setConnectedEdges (0);
        addAndMakeVisible (*button);
    }

    classic.onClick = [this] { setMode (0); };
    flex.onClick    = [this] { setMode (1); };
    dream.onClick   = [this] { setMode (2); };

    fiuso.setLookAndFeel (&knobLookAndFeel);
    fiuso.setClickingTogglesState (true);
    fiuso.setColour (juce::TextButton::buttonColourId, paper.darker (0.02f));
    fiuso.setColour (juce::TextButton::buttonOnColourId, orange);
    fiuso.setColour (juce::TextButton::textColourOffId, ink);
    fiuso.setColour (juce::TextButton::textColourOnId, cream);
    fiuso.setButtonText ("≋  FIŪSO");
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
    // Input/output trim remain automatable in the host, but the supplied
    // reference UI intentionally presents only the five main rotary controls.
    inputGain.setVisible (false);
    outputGain.setVisible (false);
    inputGainLabel.setVisible (false);
    outputGainLabel.setVisible (false);

    setMode (static_cast<int> (p.parameters.getRawParameterValue ("mode")->load()));
    animatedIntensity = juce::jlimit (0.0f, 1.0f,
        p.parameters.getRawParameterValue ("intensity")->load());
    startTimerHz (24);
}

FuturoSpringAudioProcessorEditor::~FuturoSpringAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
    presetMenu.setLookAndFeel (nullptr);
    for (auto* button : { &classic, &flex, &dream, &fiuso })
        button->setLookAndFeel (nullptr);
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
    slider.setColour (juce::Slider::trackColourId, line.withAlpha (0.75f));
    slider.setColour (juce::Slider::thumbColourId, currentModeColour());
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
    slider.setColour (juce::Slider::rotarySliderOutlineColourId, line.withAlpha (0.85f));
    slider.setColour (juce::Slider::textBoxTextColourId, ink);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, paper.withAlpha (0.0f));
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxHighlightColourId, orange.withAlpha (0.22f));
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

    character.setName (visualMode == 0 ? "SPRING" : visualMode == 1 ? "SAG" : "HALO");
    character.setTooltip (visualMode == 0
        ? "CLASSICUS: Spring — vintage spring colour and controlled harmonic drive; no pitch modulation"
        : visualMode == 1
            ? "NOVUS: Sag — Voltage Sag, a controlled level dip with a slow recovery on the reverb return"
            : "SOMNIUM: Halo — a dark, soft halo on the reverb return; fixed-time colour, no pitch modulation");

    const auto accent = currentModeColour();
    mix.setColour (juce::Slider::rotarySliderFillColourId, orange);
    saturation.setColour (juce::Slider::rotarySliderFillColourId, blue);
    intensity.setColour (juce::Slider::rotarySliderFillColourId, accent);
    character.setColour (juce::Slider::rotarySliderFillColourId, green);
    tone.setColour (juce::Slider::rotarySliderFillColourId, violet);
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

    const juce::String caption = animatedIntensity < 0.20f ? "Nostálgicos"
                               : animatedIntensity < 0.45f ? "Soñadores"
                               : animatedIntensity < 0.75f ? "Traviesos"
                                                            : "Diablillos felices";
    if (moodCaption.getText() != caption)
        moodCaption.setText (caption, juce::dontSendNotification);
    moodCaption.setColour (juce::Label::textColourId,
        animatedIntensity > 0.78f ? redGlow : currentModeColour().darker (0.08f));

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

    // Cream archival paper, a restrained copper bevel and engraved corner screws.
    juce::ColourGradient backgroundGradient (juce::Colour (0xffe4d3ad), 0.0f, 0.0f,
                                              juce::Colour (0xfff7edda),
                                              static_cast<float> (width), static_cast<float> (height), false);
    g.setGradientFill (backgroundGradient);
    g.fillAll();

    juce::ColourGradient panelGradient (juce::Colour (0xfffff7e7), outer.getX(), outer.getY(),
                                        juce::Colour (0xfff0e2c4), outer.getRight(), outer.getBottom(), false);
    g.setGradientFill (panelGradient);
    g.fillRoundedRectangle (outer, 18.0f);
    drawChalkBoardWear (g, outer);

    g.setColour (chalkHighlight.withAlpha (0.55f));
    g.drawRoundedRectangle (outer.reduced (1.0f), 18.0f, 3.2f);
    g.setColour (orange.withAlpha (0.72f));
    g.drawRoundedRectangle (outer, 18.0f, 1.0f);
    g.setColour (soft.withAlpha (0.42f));
    g.drawRoundedRectangle (outer.reduced (5.0f), 14.0f, 0.75f);

    drawVintageScrew (g, outer.getX() + 18.0f, outer.getY() + 18.0f);
    drawVintageScrew (g, outer.getRight() - 18.0f, outer.getY() + 18.0f);
    drawVintageScrew (g, outer.getX() + 18.0f, outer.getBottom() - 18.0f);
    drawVintageScrew (g, outer.getRight() - 18.0f, outer.getBottom() - 18.0f);

    // A small printed star beside the SPIRITUS wordmark and celestial marks
    // around the angel pair reproduce the illustration's hand-printed language.
    const auto drawStar = [&g] (float sx, float sy, float radius, juce::Colour colour)
    {
        juce::Path star;
        star.startNewSubPath (sx, sy - radius);
        star.lineTo (sx + radius * 0.22f, sy - radius * 0.22f);
        star.lineTo (sx + radius, sy);
        star.lineTo (sx + radius * 0.22f, sy + radius * 0.22f);
        star.lineTo (sx, sy + radius);
        star.lineTo (sx - radius * 0.22f, sy + radius * 0.22f);
        star.lineTo (sx - radius, sy);
        star.lineTo (sx - radius * 0.22f, sy - radius * 0.22f);
        star.closeSubPath();
        g.setColour (colour.withAlpha (0.82f));
        g.strokePath (star, juce::PathStrokeType (1.15f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    };
    drawStar (28.0f, 45.0f, 10.0f, orange);
    drawStar (static_cast<float> (width) * 0.50f, 48.0f, 11.0f, orange);
    drawStar (static_cast<float> (width) * 0.50f - 46.0f, 151.0f, 7.0f, blue);
    drawStar (static_cast<float> (width) * 0.50f + 60.0f, 252.0f, 8.0f, orange);
    drawStar (static_cast<float> (width) * 0.50f, 369.0f, 9.0f, orange);

    // Etched blue-cloud curls: delicate enough to sit behind the crayons.
    juce::Path cloudLeft;
    cloudLeft.startNewSubPath (246.0f, 347.0f);
    cloudLeft.cubicTo (258.0f, 331.0f, 275.0f, 335.0f, 282.0f, 344.0f);
    cloudLeft.cubicTo (292.0f, 326.0f, 316.0f, 329.0f, 326.0f, 345.0f);
    cloudLeft.cubicTo (342.0f, 335.0f, 359.0f, 343.0f, 364.0f, 354.0f);
    cloudLeft.cubicTo (350.0f, 369.0f, 273.0f, 371.0f, 246.0f, 347.0f);
    juce::Path cloudRight;
    cloudRight.startNewSubPath (516.0f, 351.0f);
    cloudRight.cubicTo (530.0f, 335.0f, 549.0f, 339.0f, 558.0f, 352.0f);
    cloudRight.cubicTo (571.0f, 337.0f, 590.0f, 344.0f, 599.0f, 358.0f);
    cloudRight.cubicTo (584.0f, 373.0f, 535.0f, 370.0f, 516.0f, 351.0f);
    g.setColour (blue.withAlpha (0.20f));
    g.strokePath (cloudLeft, juce::PathStrokeType (1.65f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
    g.strokePath (cloudRight, juce::PathStrokeType (1.65f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    juce::Path cloudCurl;
    cloudCurl.startNewSubPath (267.0f, 362.0f);
    cloudCurl.cubicTo (281.0f, 350.0f, 293.0f, 361.0f, 288.0f, 368.0f);
    cloudCurl.cubicTo (283.0f, 375.0f, 272.0f, 368.0f, 280.0f, 364.0f);
    cloudCurl.startNewSubPath (544.0f, 365.0f);
    cloudCurl.cubicTo (557.0f, 353.0f, 570.0f, 364.0f, 565.0f, 371.0f);
    cloudCurl.cubicTo (560.0f, 378.0f, 550.0f, 371.0f, 557.0f, 367.0f);
    g.setColour (blue.withAlpha (0.24f));
    g.strokePath (cloudCurl, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));

    // Technical target marks around the characters.
    const float cx = static_cast<float> (width) * 0.50f;
    const float cy = 229.0f;
    const float ringBreath = 1.0f + 0.025f * breathe * (0.25f + mood);
    const float ringW = (178.0f + mood * 24.0f) * ringBreath;
    const float ringH = (210.0f + mood * 28.0f) * ringBreath;

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
    drawChalkTexture (g, peachWing, rose, 0x50454143u, 190, 0.30f);
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
    drawChalkTexture (g, blueBody, blue, 0x424c5545u, 265, 0.30f);

    // SOMNIUM gives the blue angel a worn brass astronaut helmet with a glassy,
    // chalk-blue visor. The face remains visible under the translucent glass.
    if (visualMode == 2)
    {
        g.setColour (charcoal.withAlpha (0.95f));
        g.fillEllipse (-10.0f, -108.0f, 61.0f, 61.0f);
        juce::Path helmetGlass;
        helmetGlass.addEllipse (-6.0f, -104.0f, 53.0f, 53.0f);
        g.setColour (blue.withAlpha (0.16f));
        g.fillPath (helmetGlass);
        drawChalkTexture (g, helmetGlass, blue, 0x48454c4du, 86, 0.12f);
        g.setColour (orange.withAlpha (0.96f));
        g.drawEllipse (-10.0f, -108.0f, 61.0f, 61.0f, 2.0f);
        g.setColour (cream.withAlpha (0.88f));
        g.drawEllipse (-5.0f, -103.0f, 51.0f, 51.0f, 1.0f);
        g.setColour (orange.withAlpha (0.82f));
        g.drawLine (0.0f, -53.0f, 34.0f, -53.0f, 2.7f);
        g.setColour (cream.withAlpha (0.65f));
        g.drawLine (-2.0f, -97.0f, 7.0f, -89.0f, 1.1f);
        g.drawLine (42.0f, -86.0f, 45.0f, -78.0f, 0.9f);
        g.setColour (orange.withAlpha (0.90f));
        g.fillEllipse (-8.0f, -81.0f, 3.6f, 3.6f);
        g.fillEllipse (47.0f, -81.0f, 3.6f, 3.6f);
        // A tiny oxygen hose coils down toward the blue suit.
        juce::Path oxygenHose;
        oxygenHose.startNewSubPath (44.0f, -57.0f);
        oxygenHose.cubicTo (57.0f, -53.0f, 61.0f, -41.0f, 52.0f, -35.0f);
        g.setColour (teal.withAlpha (0.92f));
        g.strokePath (oxygenHose, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                        juce::PathStrokeType::rounded));
        
    }

    // NOVUS is marked by unmistakable bat wings; other modes keep the
    // charcoal feathered wing from the original crayon drawing.
    g.saveState();
    g.addTransform (juce::AffineTransform (wingOpen, 0.0f, 31.0f * (1.0f - wingOpen),
                                           0.0f, 1.0f, 0.0f));
    if (visualMode == 1)
    {
        juce::Path batWing;
        batWing.startNewSubPath (29.0f, -33.0f);
        batWing.cubicTo (44.0f, -56.0f, 62.0f, -55.0f, 76.0f, -42.0f);
        batWing.lineTo (84.0f, -60.0f);
        batWing.lineTo (92.0f, -40.0f);
        batWing.lineTo (111.0f, -39.0f);
        batWing.lineTo (100.0f, -20.0f);
        batWing.lineTo (107.0f, -4.0f);
        batWing.lineTo (85.0f, -9.0f);
        batWing.lineTo (73.0f, 7.0f);
        batWing.lineTo (58.0f, -8.0f);
        batWing.lineTo (43.0f, -5.0f);
        batWing.lineTo (31.0f, 2.0f);
        batWing.cubicTo (25.0f, -9.0f, 23.0f, -22.0f, 29.0f, -33.0f);
        batWing.closeSubPath();
        g.setColour (demonWing);
        g.fillPath (batWing);
        g.setColour (redGlow.withAlpha (0.94f));
        g.strokePath (batWing, juce::PathStrokeType (1.9f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
        drawChalkTexture (g, batWing, redGlow, 0x4e4f5653u, 180, 0.27f);
        juce::Path wingRibs;
        wingRibs.startNewSubPath (32.0f, -31.0f);
        wingRibs.lineTo (84.0f, -51.0f);
        wingRibs.lineTo (92.0f, -38.0f);
        wingRibs.startNewSubPath (39.0f, -19.0f);
        wingRibs.lineTo (100.0f, -21.0f);
        wingRibs.startNewSubPath (35.0f, -8.0f);
        wingRibs.lineTo (86.0f, -9.0f);
        wingRibs.lineTo (73.0f, 5.0f);
        g.setColour (orange.withAlpha (0.94f));
        g.strokePath (wingRibs, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
    }
    else
    {
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
        drawChalkTexture (g, darkWing, charcoal, 0x4348414cu, 155, 0.29f);
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
    }
    g.restoreState();

    // Expression interpolation: tearful frown at minimum, grin and horns at maximum.
    // Eyes remain tiny black dots to preserve the look of the hand-drawn characters.
    g.setColour (faceInk);
    g.fillEllipse (16.0f, -80.0f, 4.7f, 4.7f);
    g.setColour (cream.withAlpha (0.75f));
    g.fillEllipse (17.0f, -79.0f, 1.15f, 1.15f);

    juce::Path blueMouth;
    blueMouth.startNewSubPath (12.0f, -68.0f);
    blueMouth.cubicTo (14.0f, -63.0f - happy * 12.0f,
                       20.0f, -63.0f - happy * 12.0f, 22.0f, -68.0f);
    g.setColour (faceInk);
    g.strokePath (blueMouth, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));

    juce::Path blueBrow;
    blueBrow.startNewSubPath (12.0f, -86.0f);
    blueBrow.lineTo (21.0f, -82.5f - sad * 3.4f + devil * 2.2f);
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
    drawChalkTexture (g, greenBody, green, 0x47524545u, 230, 0.30f);

    juce::Path embraceArm;
    embraceArm.startNewSubPath (-37.0f, 4.0f);
    embraceArm.cubicTo (-24.0f, -1.0f, -13.0f, -18.0f, -2.0f, -31.0f);
    embraceArm.cubicTo (8.0f, -43.0f, 17.0f, -48.0f, 23.0f, -41.0f);
    embraceArm.cubicTo (28.0f, -35.0f, 17.0f, -27.0f, 9.0f, -19.0f);
    embraceArm.cubicTo (-2.0f, -7.0f, -15.0f, 8.0f, -29.0f, 17.0f);
    embraceArm.cubicTo (-36.0f, 21.0f, -42.0f, 15.0f, -37.0f, 4.0f);
    embraceArm.closeSubPath();
    g.fillPath (embraceArm);
    drawChalkTexture (g, embraceArm, green, 0x41524d43u, 115, 0.28f);

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
    greenMouth.cubicTo (-42.0f, -22.0f - happy * 12.0f,
                        -36.0f, -22.0f - happy * 12.0f, -34.0f, -27.0f);
    g.setColour (faceInk);
    g.strokePath (greenMouth, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));

    juce::Path greenBrow;
    greenBrow.startNewSubPath (-44.0f, -45.0f);
    greenBrow.lineTo (-35.0f, -42.0f - sad * 3.4f + devil * 2.2f);
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

    // Small engraved rules frame the live mood label without competing with the angels.
    g.setColour (orange.withAlpha (0.55f));
    g.drawLine (cx - 118.0f, 412.0f, cx - 82.0f, 412.0f, 0.9f);
    g.drawLine (cx + 82.0f, 412.0f, cx + 118.0f, 412.0f, 0.9f);
    drawStar (cx - 70.0f, 412.0f, 3.8f, orange);
    drawStar (cx + 70.0f, 412.0f, 3.8f, orange);

}

void FuturoSpringAudioProcessorEditor::resized()
{
    const int width = getWidth();
    const int height = getHeight();

    heading.setBounds (36, 20, 220, 38);
    subheading.setBounds (40, 57, 220, 18);

    // Three compact tabs centred in the header, just like spiritus_v2.html.
    const int modeWidth = 112;
    const int modeGap = 4;
    const int modeLeft = width / 2 - (modeWidth * 3 + modeGap * 2) / 2;
    classic.setBounds (modeLeft, 22, modeWidth, 38);
    flex.setBounds (modeLeft + modeWidth + modeGap, 22, modeWidth, 38);
    dream.setBounds (modeLeft + 2 * (modeWidth + modeGap), 22, modeWidth, 38);

    // Main controls orbit the centre illustration instead of forming a bottom strip.
    mix.setBounds (97, 120, 106, 134);
    saturation.setBounds (97, 285, 106, 134);
    character.setBounds (677, 120, 106, 134);
    tone.setBounds (677, 285, 106, 134);
    intensity.setBounds (365, 436, 150, 166);

    moodCaption.setBounds (320, 399, 240, 23);

    presetLabel.setBounds (40, 475, 200, 16);
    presetMenu.setBounds (40, 493, 200, 42);
    fiuso.setBounds (640, 512, 200, 42);

    // These remain attached to APVTS and available to host automation, but are
    // not shown in this exact reference layout.
    inputGainLabel.setBounds (0, 0, 1, 1);
    outputGainLabel.setBounds (0, 0, 1, 1);
    inputGain.setBounds (0, 0, 1, 1);
    outputGain.setBounds (0, 0, 1, 1);
}
