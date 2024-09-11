#include "JuceHeader.h"
#include "PedalComponent.h"

//==============================================================================
// Implementation of PedalKnobLookAndFeel to customize knob appearance

void PedalKnobLookAndFeel::drawRotarySlider(
    juce::Graphics& g,
    int x,
    int y,
    int width,
    int height,
    float sliderPosProportional,
    float rotaryStartAngle,
    float rotaryEndAngle,
    juce::Slider& slider)
{
    juce::Rectangle<int> bounds(x, y, width, height);
    const auto centre = bounds.getCentre().toFloat();

    // Check if the slider is the tremolo knob and apply custom styling
    if (slider.getName() == "Tremolo")
    {
        // Draw the black knob body
        juce::Rectangle<float> knobBounds = bounds.toFloat().reduced(2.0f); // Reduce to create some padding
        g.setColour(juce::Colours::black);
        g.fillEllipse(knobBounds);

        // Apply gradient to the tremolo knob similar to the other knobs
        juce::ColourGradient gradient;
        gradient.point1 = centre.withY(bounds.getY()).toFloat();
        gradient.point2 = centre.withX(bounds.getBottom()).toFloat();
        gradient.addColour(1.0, juce::Colours::black.brighter(0.2f));
        gradient.addColour(0.1, juce::Colours::black.darker(0.2f));
        gradient.isRadial = true;
        g.setGradientFill(gradient);
        g.fillEllipse(bounds.toFloat());

        // Ensure the outline is a complete circle
        g.setColour(juce::Colours::black.brighter(0.1F));
        g.drawEllipse(knobBounds, 1.5f); //thickness for the outline

        // Draw a standard knob indicator
        const float length = bounds.expanded(5).getWidth();
        const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        const float outerRadius = knobBounds.getWidth() / 2.0f - 1.5f; // Subtract 3 to ensure it stays within outline
        const float innerRadius = outerRadius * 0.4f;  // Shorter inner radius to stay within bounds

        const juce::Point<float> startPoint = knobBounds.getCentre().getPointOnCircumference(innerRadius, angle);
        const juce::Point<float> endPoint = knobBounds.getCentre().getPointOnCircumference(outerRadius, angle);

        g.setColour(juce::Colours::yellow.darker(0.1f));
        g.drawLine({ startPoint, endPoint }, 2.0f); // Standard indicator
    }
    else
    {
        // Normal knob drawing for other knobs (Drive, Level)
        juce::ColourGradient gradient;
        gradient.point1 = centre.withY(bounds.getY()).toFloat();
        gradient.point2 = centre.withX(bounds.getBottom()).toFloat();
        gradient.addColour(0.0, juce::Colours::white.withAlpha(0.2f));
        gradient.addColour(1.0, juce::Colours::transparentWhite);
        gradient.isRadial = true;
        g.setGradientFill(gradient);
        g.fillEllipse(bounds.toFloat());

        bounds = bounds.reduced(2);

        gradient.clearColours();
        gradient.point1 = centre;
        gradient.point2 = centre.withX(0).toFloat();
        gradient.addColour(0.0, juce::Colours::black);
        gradient.addColour(1.0, juce::Colours::black.brighter(0.2f));
        gradient.isRadial = true;
        g.setGradientFill(gradient);
        g.fillEllipse(bounds.toFloat());

        bounds = bounds.reduced(3);

        const auto thumbColour = slider
            .findColour(juce::Slider::thumbColourId)
            .withMultipliedSaturation(1.10f)
            .withMultipliedBrightness(1.85f);

        g.setColour(thumbColour);
        g.fillEllipse(bounds.toFloat());

        gradient.clearColours();
        gradient.point1 = centre.withY(bounds.getY()).toFloat();
        gradient.point2 = centre.withX(bounds.getBottom()).toFloat();
        gradient.addColour(0.0, thumbColour.brighter());
        gradient.addColour(1.0, thumbColour.darker());
        gradient.isRadial = true;
        g.setGradientFill(gradient);
        g.fillEllipse(bounds.toFloat());

        bounds = bounds.reduced(2);

        gradient.clearColours();
        gradient.point1 = centre.withY(bounds.getY()).toFloat();
        gradient.point2 = centre.withX(bounds.getBottom()).toFloat();
        gradient.addColour(0.0, thumbColour);
        gradient.addColour(1.0, thumbColour.darker(0.4f));
        gradient.isRadial = true;
        g.setGradientFill(gradient);
        g.fillEllipse(bounds.toFloat());

        // Draw Knob Indicator
        const float length = bounds.expanded(4).getWidth();
        const float angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

        const juce::Line<float> indicator(
            bounds.getCentre().getPointOnCircumference(length / 2.0f, angle),
            bounds.getCentre().getPointOnCircumference(length / 5.0f, angle)
        );

        g.setColour(juce::Colours::black.brighter(0.1f));
        g.drawLine(indicator, 5);
    }
}

//==============================================================================
// PedalComponent Constructor and Methods

PedalComponent::PedalComponent(DisruptionAudioProcessor& p,
    const juce::StringRef& name,
    const juce::StringRef& shortName,
    const juce::Colour& colour,
    const KnobNames& knobNames)
    : AudioProcessorEditor(&p), processor(p), colour(colour), tremoloKnobVisible(false)
{
    setSize(300, 500); // Set the size of the plugin window

    // Load images from resources
    boltOffImage = juce::ImageCache::getFromMemory(BinaryData::boltOff_png, BinaryData::boltOff_pngSize);
    boltOnImage = juce::ImageCache::getFromMemory(BinaryData::boltOn_png, BinaryData::boltOn_pngSize);
    disruptionLogoImage = juce::ImageCache::getFromMemory(BinaryData::disruptionlogo_png, BinaryData::disruptionlogo_pngSize);

    // Load Font from BinaryData
    juce::Typeface::Ptr customTypeface = juce::Typeface::createSystemTypefaceFor(BinaryData::fightingspirittbs_regular_ttf, BinaryData::fightingspirittbs_regular_ttfSize);
    juce::Font customFont(customTypeface.get());

    // Initialize knobs
    for (int i = 0; i < 2; ++i)
    {
        knobs[i].setSliderStyle(juce::Slider::Rotary);
        knobs[i].setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
        knobs[i].setRange(0.0, 1.0);
        knobs[i].setValue((i == 0) ? processor.getDistortionValue() : processor.getLevelValue());
        knobs[i].setColour(juce::Slider::rotarySliderFillColourId, colour);
        knobs[i].setColour(juce::Slider::thumbColourId, juce::Colours::white);
        knobs[i].setLookAndFeel(lookAndFeel);
        knobs[i].addListener(this);
        addAndMakeVisible(knobs[i]);
    }

    knobs[0].setName(knobNames.first); 
    knobs[1].setName(knobNames.second);

    // Initialize the tremolo knob (third knob)
    tremoloKnob.setName("Tremolo");
    tremoloKnob.setSliderStyle(juce::Slider::Rotary);
    tremoloKnob.setTextBoxStyle(juce::Slider::NoTextBox, true, 0, 0);
    tremoloKnob.setRange(0.1, 10.0); // Adjust the range as needed
    tremoloKnob.setValue(5.0); // Default value
    tremoloKnob.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::black);
    tremoloKnob.setColour(juce::Slider::thumbColourId, juce::Colours::yellow);
    tremoloKnob.setLookAndFeel(lookAndFeel);
    tremoloKnob.addListener(this);
    tremoloKnob.setVisible(false); // Start hidden
    addAndMakeVisible(tremoloKnob);

    // Initialize the label for the tremolo knob
    tremoloLabel.setFont(customFont); // Use the custom font
    tremoloLabel.setText("DISRUPTION", juce::dontSendNotification);
    tremoloLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    tremoloLabel.setJustificationType(juce::Justification::centred);
    tremoloLabel.setVisible(false); // Start hidden
    addAndMakeVisible(tremoloLabel);

    // Enable buffered image for performance
    setBufferedToImage(true);
}

// Destructor definition
PedalComponent::~PedalComponent()
{
    knobs[0].setLookAndFeel(nullptr);
    knobs[1].setLookAndFeel(nullptr);
    tremoloKnob.setLookAndFeel(nullptr);
}

void PedalComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::midnightblue); // Pedal background color

    // Draw pedal body
    g.setColour(colour);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 10.0f);

    // Call method to draw pedal decorations like rubber area and shadows
    drawPedalDecorations(g);

    // Calculate buttonArea again
    auto bounds = getLocalBounds().reduced(15);
    auto buttonArea = bounds.removeFromBottom(175);
  
    // Define the desired width for the logo
    const int desiredLogoWidth = 219; // Set desired width
    const float aspectRatio = disruptionLogoImage.getWidth() / (float)disruptionLogoImage.getHeight();
    const int logoHeight = static_cast<int>(desiredLogoWidth / aspectRatio);

    // Calculate the position for centering the logo at the top 10% of the buttonArea
    juce::Rectangle<int> logoBounds(
        buttonArea.getCentreX() - (desiredLogoWidth / 2), // Center horizontally
        buttonArea.getY() + (buttonArea.getHeight() * 0.09f), // from the top of buttonArea
        desiredLogoWidth,
        logoHeight
    );

    // Draw the logo image centered at the top 10% of the buttonArea
    g.drawImageWithin(disruptionLogoImage,
        logoBounds.getX(), logoBounds.getY(),
        logoBounds.getWidth(), logoBounds.getHeight(),
        juce::RectanglePlacement::centred);

    // Draw indicator light based on button state
    juce::Image lightImage = isLightOn ? boltOnImage : boltOffImage;
    auto lightArea = juce::Rectangle<int>(getWidth() / 2 - 15, 10, 30, 30);
    g.drawImageWithin(lightImage, lightArea.getX(), lightArea.getY(), lightArea.getWidth(), lightArea.getHeight(), juce::RectanglePlacement::centred);

    // Draw Knob Labels
    g.setColour(juce::Colours::white);
    g.setFont(16.0f);
    for (const auto& knob : knobs)
    {
        g.drawText(
            knob.getName(),
            knob.getBounds().translated(0, 32),
            juce::Justification::centredBottom
        );
    }
}

void PedalComponent::mouseUp(const juce::MouseEvent& e)
{
    auto area = getLocalBounds().reduced(10);

    if (area.contains(e.getPosition()))
    {
        // Toggle the effect on/off state
        processor.setEffectOn(!processor.isEffectOn());        
        tremoloKnobVisible = !tremoloKnobVisible; // Toggle visibility
        tremoloKnob.setVisible(tremoloKnobVisible);
        tremoloLabel.setVisible(tremoloKnobVisible);
        processor.setTremoloOn(tremoloKnobVisible); // Enable or disable tremolo effect based on knob visibility
        isLightOn = !isLightOn; // Toggle the light state
        resized(); // Update layout when visibility changes
        repaint(); // Repaint to update light

    }
}

// New method to handle pedal decorations (shadows, rubber areas, etc.)
void PedalComponent::drawPedalDecorations(juce::Graphics& g)
{
    auto bounds = getLocalBounds().reduced(15);

    // Draw rubber area or foot area
    auto buttonArea = bounds.removeFromBottom(175);
    juce::ColourGradient gradient;
    gradient.point1 = buttonArea.getBottomLeft().toFloat();
    gradient.point2 = buttonArea.getTopLeft().toFloat();
    gradient.addColour(0.0, juce::Colours::black.withAlpha(0.33f));
    gradient.addColour(0.1, juce::Colours::transparentWhite);
    gradient.addColour(0.9, juce::Colours::transparentBlack);
    gradient.addColour(1.0, juce::Colours::black.withAlpha(0.33f));
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(buttonArea.toFloat(), 12.0f);

    buttonArea = buttonArea.reduced(2);

    g.setColour(juce::Colours::black.brighter(0.2f));
    g.fillRoundedRectangle(buttonArea.toFloat(), 12.0f);

    gradient.clearColours();
    gradient.point1 = buttonArea.getBottomLeft().toFloat();
    gradient.point2 = buttonArea.getTopLeft().toFloat();
    gradient.addColour(0.0, juce::Colours::black);
    gradient.addColour(0.9, juce::Colours::black.withAlpha(0.5f));
    gradient.addColour(1.0, juce::Colours::white.withAlpha(0.25f));
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(buttonArea.toFloat(), 12.0f);

    buttonArea = buttonArea.reduced(2);

    gradient.clearColours();
    gradient.point1 = buttonArea.getBottomLeft().toFloat();
    gradient.point2 = buttonArea.getTopLeft().toFloat();
    gradient.addColour(0.0, juce::Colours::black.brighter(0.1f));
    gradient.addColour(1.0, juce::Colours::black.brighter(0.2f));
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(buttonArea.toFloat(), 12.0f);

    buttonArea = buttonArea.removeFromBottom(75).reduced(15);

    for (float ratio = 0.0f; ratio < 1.0f; ratio += 0.33f)
    {
        g.setColour(juce::Colours::white.withAlpha(0.33f));
        g.drawHorizontalLine(
            buttonArea.getY() + buttonArea.getHeight() * ratio,
            buttonArea.getX(),
            buttonArea.getRight()
        );

        g.setColour(juce::Colours::black.withAlpha(0.66f));
        g.drawHorizontalLine(
            buttonArea.getY() + 1 + buttonArea.getHeight() * ratio,
            buttonArea.getX(),
            buttonArea.getRight()
        );
        g.drawHorizontalLine(
            buttonArea.getY() - 1 + buttonArea.getHeight() * ratio,
            buttonArea.getX(),
            buttonArea.getRight()
        );
    }

    // Draw shadows and other effects around the foot area and knobs
    drawShadows(g, bounds);
}

void PedalComponent::drawShadows(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    int shadowStartY = bounds.getY() - 15; // Set this to the desired starting Y coordinate for shadows

    // Move the bounds' Y position to shadowStartY and adjust its height accordingly
    bounds.setY(shadowStartY);

    // Draw shadows or other graphical effects
    for (int i = 0; i < 2; ++i)
    {
        auto lightingArea = bounds
            .removeFromBottom(20 - (i * 7))
            .withX(1)
            .withRight(getWidth() - 1);

        juce::Path roundedTop;
        roundedTop.addRoundedRectangle(
            lightingArea.getX(), lightingArea.getY(),
            lightingArea.getWidth(), lightingArea.getHeight(),
            12.0f, 12.0f, // cornerSizeX, cornerSizeY
            i == 0,       // curveTopLeft
            i == 0,       // curveTopRight
            i == 1,       // curveBottomLeft
            i == 1        // curveBottomRight
        );

        juce::ColourGradient gradient;
        gradient.point1 = lightingArea.getTopLeft().toFloat();
        gradient.point2 = lightingArea.getBottomLeft().toFloat();
        gradient.addColour(0.0, juce::Colours::transparentWhite);
        gradient.addColour(0.5, juce::Colours::white.withAlpha(0.19f));
        gradient.addColour(1.0, juce::Colours::transparentWhite);
        g.setGradientFill(gradient);
        g.fillPath(roundedTop);
    }
}

void PedalComponent::resized()
{
    auto area = getLocalBounds().removeFromTop(160);

    // FlexBox layout for knobs
    juce::FlexBox flexbox;
    flexbox.justifyContent = juce::FlexBox::JustifyContent::spaceAround;
    flexbox.alignContent = juce::FlexBox::AlignContent::center;
    flexbox.alignItems = juce::FlexBox::AlignItems::center;

    for (auto& knob : knobs)
        flexbox.items.add(juce::FlexItem(85.0f, 85.0f, knob));

    flexbox.performLayout(area);

    // Center the third knob below the first two if it's visible
    if (tremoloKnobVisible)
    {
        // Calculate the center position between the two knobs
        auto driveKnobBounds = knobs[0].getBounds();
        auto levelKnobBounds = knobs[1].getBounds();

        // Determine the horizontal center between the two knobs
        int centerX = (driveKnobBounds.getRight() + levelKnobBounds.getX()) / 2;

        // Set the size and position of the tremolo knob
        auto tremoloKnobSize = driveKnobBounds.getWidth() * 0.75f; // 75% of the size of the other knobs
        tremoloKnob.setBounds(
            centerX - (tremoloKnobSize / 2), // Center it horizontally
            driveKnobBounds.getBottom() + 20, // Position it below the first knob
            tremoloKnobSize,
            tremoloKnobSize
        );
   
        // Calculate the width needed for the label text
        int textWidth = tremoloLabel.getFont().getStringWidth(tremoloLabel.getText());
        textWidth += 10; // Add some padding for visual comfort

        // Update the label's bounds with the new width
        tremoloLabel.setBounds(
            tremoloKnob.getX() - (textWidth - tremoloKnob.getWidth()) / 2, // Center align the label with the knob
            tremoloKnob.getBottom() + 10, // Position the label below the knob
            textWidth, // Width to fit the text
            20 // Height for the label
        );
    }
}

void PedalComponent::sliderValueChanged(juce::Slider* slider) {
    if (slider == &knobs[0]) {
        processor.setDistortionValue(knobs[0].getValue());  // Use the setter
    }
    if (slider == &knobs[1]) {
        processor.setLevelValue(knobs[1].getValue());  // Use the setter
    }

    {
        // Handle tremolo knob changes
        processor.setTremoloRate(tremoloKnob.getValue());
    }
    repaint(); // Trigger repaint to update the UI
}
