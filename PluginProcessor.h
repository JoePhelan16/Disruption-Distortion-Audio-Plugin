#pragma once

#include <JuceHeader.h>
#include "PedalComponent.h"

//==============================================================================
class DisruptionAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    explicit DisruptionAudioProcessor();
    ~DisruptionAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void releaseResources() override;

    // Getter and setter for distortion value (drive knob)
    float getDistortionValue() const { return distortionValue; }
    void setDistortionValue(float newValue) { distortionValue = newValue; setDistortionKnob(newValue); }

    // Getter and setter for level value
    float getLevelValue() const { return levelValue; }
    void setLevelValue(float newValue) { levelValue = newValue; setClippingKnob(newValue); }


    // Methods to prepare distortion and clipping with sample rate
    void prepareDistortion(float newFs);
    void prepareClipping(float newFS);

    // Getter and setter for effectOn
    bool isEffectOn() const { return effectOn; }
    void setEffectOn(bool isOn) { effectOn = isOn; }

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Tremolo effect parameters
    float getTremoloRate() const { return tremoloRate; }
    void setTremoloRate(float newRate); // Only declare here

    bool isTremoloOn() const { return tremoloOn; }
    void setTremoloOn(bool isOn) { tremoloOn = isOn; }


private:
    //==============================================================================
    // Sample rate and time variables
    float Fs;  // Sample rate
    float inverseSampleRate;
    float Ts;  // Sampling period

    // Declare an array to store the last sample for each channel
    std::vector<float> lastSample;  // Initialize with 0.0f for each channel

    //==============================================================================
    // Distortion-related parameters and methods
    float distortionValue = 0.5f;  // Default distortion value
    float C1;  // Capacitance for distortion circuit
    float R1;  // Resistance R1
    float x1;  // State variable for distortion
    float R3;  // Resistance R3
    float R4;  // Resistance R4
    float potDis;  // Distortion knob value
    float Rp;  // Potentiometer resistance for distortion
    float G_distortion; // Conductance for distortion
    float Gb;  // Conductance for branch B
    float Gi;  // Input conductance
    float Gx1; // Conductance for x1

    void updateDistortionCoefficients();  // Update coefficients for distortion
    void updateDistortionGroupedResistances();  // Update grouped resistances for distortion

    //==============================================================================
    // Clipping-related parameters and methods
    void updateClippingCoefficients();  // Update coefficients for clipping
    void updateClippingGroupedResistances();  // Update grouped resistances for clipping
    float levelValue = 0.5f;       // Default level value

    float C2;  // Capacitance for clipping circuit
    float R2;  // Resistance R2
    float x2;  // State variable for clipping
    float R5;  // Resistance R5
    float potLev;  // Output level control
    float Vd;  // Voltage across the diode
    float thr; // Threshold for convergence

    const float eta = 2.f;  // Diode emission coefficient
    const float Is = 1.e-6;  // Reverse saturation current
    const float Vt = 26.e-3;  // Thermal voltage
    float G_clipping;   // Conductance for clipping

    // Declare state variables for each channel
    std::vector<float> x1State;  // State variable for distortion
    std::vector<float> x2State;  // State variable for clipping (if needed)


    //==============================================================================
    // Effect control
    bool effectOn;
    void setDistortionKnob(float disKnob);  // Update distortion knob
    void setClippingKnob(float levelKnob);  // Update clipping knob

    // Distortion and Clipping processing
    float processDistortionSample(float Vi, float& x1);
    float processClippingSample(float Vi, float& x2);


    //==============================================================================
    // Tremolo-related parameters
    float tremoloRate;
    bool tremoloOn;
    double tremoloPhase;
    float tremoloDepth;

    //==============================================================================
    // Chorus effect
    juce::dsp::Chorus<float> chorus;

    //==============================================================================
    // Filters
     /*juce::IIRFilter highPassFilter;  // High-pass filter */
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> iir;  // Low-pass filter

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DisruptionAudioProcessor)
};
