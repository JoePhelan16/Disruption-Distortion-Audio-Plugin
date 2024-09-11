#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

using KnobNames = std::pair<juce::StringRef, juce::StringRef>;

class PedalKnobLookAndFeel : public juce::LookAndFeel_V4 {
public:
    PedalKnobLookAndFeel() = default;
    ~PedalKnobLookAndFeel() override = default;

    void drawRotarySlider(
        juce::Graphics& g,
        int x,
        int y,
        int width,
        int height,
        float sliderPosProportional,
        float rotaryStartAngle,
        float rotaryEndAngle,
        juce::Slider& slider
    ) override;

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PedalKnobLookAndFeel)
};

class DisruptionAudioProcessor;

class PedalComponent : public juce::AudioProcessorEditor, public juce::Slider::Listener {
public:
    PedalComponent(DisruptionAudioProcessor& p,
                   const juce::StringRef& name,
                   const juce::StringRef& shortName,
                   const juce::Colour& colour,
                   const KnobNames& knobs);

    ~PedalComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void sliderValueChanged(juce::Slider* slider) override;

private:
    void mouseUp(const juce::MouseEvent& e) override;
    void drawPedalDecorations(juce::Graphics& g);
    void drawShadows(juce::Graphics& g, juce::Rectangle<int> bounds);

    juce::SharedResourcePointer<PedalKnobLookAndFeel> lookAndFeel;

    juce::Colour colour;
    juce::Slider knobs[2]; // Knobs for Drive and Level
    juce::Slider tremoloKnob; // Tremolo knob (third knob)
    juce::Label tremoloLabel; // Label for the tremolo knob
    bool tremoloKnobVisible = false; // To track if the tremolo knob should be visible

    DisruptionAudioProcessor& processor;
    
    // Member variable for button area
    juce::Rectangle<int> buttonArea;

    // Add image for indicator 
    juce::Image boltOffImage;
    juce::Image boltOnImage;
    juce::Image disruptionLogoImage; // Add image for disruption logo
    bool isLightOn = false; // Track the light state
    
    // Custom font member for the tremolo label
    std::unique_ptr<juce::Typeface> customFont;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PedalComponent)
};
