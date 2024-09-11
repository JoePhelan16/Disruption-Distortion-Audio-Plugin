#include "PluginProcessor.h"
#include "PedalComponent.h"
#include "JuceHeader.h"

//==============================================================================
DisruptionAudioProcessor::DisruptionAudioProcessor()
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ),

    Fs(44100.f),  // Initialize sample rate to a default value (will be updated in prepareToPlay)
    inverseSampleRate(1.0f / Fs),  // Inverse of sample rate
    Ts(1.f / Fs),  // Sampling period

    // Initialize distortion-related parameters
    distortionValue(0.5f),  // Initialize distortion value (drive knob)
    C1(47.e-9f),
    R1(Ts / (2.f * C1)),
    R3(4.7e3f),
    R4(1.e6f),
    potDis(0.f),
    Rp(0.0f),
    G_distortion(0.0f),
    Gb(0.0f),
    Gi(0.0f),
    Gx1(0.0f),
    x1(0.0f),

    // Initialize clipping-related parameters
    levelValue(0.5f),       // Initialize level value
    C2(1.e-9f),
    R5(10.e3f),
    Vd(0.0f),
    thr(0.0000001f),
    R2(0.0f),
    x2(0.0f),
    G_clipping(0.0f),
    potLev(0.f),        // Initialize level knob state

    // Initialize effect control values
    effectOn(true),

    // Initialize tremolo parameters
    tremoloRate(2.0f),
    tremoloOn(false),
    tremoloPhase(0.0),
    tremoloDepth(0.3f),

    // Initialize chorus effect
    chorus(),

     //Initialize high-pass and low-pass filters
    /*highPassFilter(),*/
    iir()
{
    // Initialize lastSample vector for channel-based processing
    lastSample.resize(getTotalNumInputChannels(), 0.0f);

    // Prepare the chorus effect with default values
    chorus.setRate(0.5f);
    chorus.setDepth(0.2f);
    chorus.setCentreDelay(3.0f);
    chorus.setFeedback(0.2f);
    chorus.setMix(0.3f);
}

// Destructor definition
DisruptionAudioProcessor::~DisruptionAudioProcessor()
{
}

//==============================================================================
void DisruptionAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    Fs = static_cast<float>(sampleRate);
    inverseSampleRate = 1.0f / Fs;          // Calculate and store the inverse of the sample rate
    Ts = inverseSampleRate;
    updateDistortionCoefficients();
    updateClippingCoefficients();
   
    // Resize lastSample to match input channels
    lastSample.resize(getTotalNumInputChannels(), 0.0f);

    // Initialize or resize x1State for each channel (same for x2State if needed)
    x1State.resize(getTotalNumInputChannels(), 0.0f);
    x2State.resize(getTotalNumInputChannels(), 0.0f);  // If needed for clipping
    // Create a single ProcessSpec instance to use for all DSP initialization
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 2;  // Ensure that stereo is supported in the DSP setup


     //Prepare the low - pass filter
    iir.state = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 5000.0);
    iir.prepare(spec);

    /* Initialize high - pass filter with a cutoff frequency of 200 Hz
    double cutoffFrequency = 0.0;  // Change this value as needed
    auto highPassCoefficients = juce::IIRCoefficients::makeHighPass(sampleRate, cutoffFrequency);
    highPassFilter.setCoefficients(highPassCoefficients); */

    // Prepare the chorus effect
    chorus.prepare(spec);

    // Initialize tremolo parameters
    tremoloPhase = 0.0;
}

//==============================================================================
void DisruptionAudioProcessor::releaseResources()
{
    // Cleanup if needed
}

bool DisruptionAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}


void DisruptionAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = buffer.getNumChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    // Set knob values for distortion and clipping
    setDistortionKnob(distortionValue);  // Update the distortion knob value
    setClippingKnob(levelValue);

    // Clear any unused output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    auto numSamples = buffer.getNumSamples();
    auto sampleRate = getSampleRate();

    // Process each channel
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        const float* inputChannelData = buffer.getReadPointer(channel < totalNumInputChannels ? channel : 0);

        for (int n = 0; n < numSamples; ++n)
        {
            // Get the input sample and use the last sample for this channel
            float inputSample = inputChannelData[n];

            // Apply distortion
            float distortedSample = processDistortionSample(inputSample, x1State[channel]);

            // Apply clipping
            float clippedSample = processClippingSample(distortedSample, x2State[channel]);

            // Apply tremolo if enabled
            if (tremoloOn)
            {
                // Simple tremolo effect using LFO (Low-Frequency Oscillator)
                float lfo = std::sin(tremoloPhase) >= 0 ? 1.0f : -1.0f;  // Square wave
                float tremoloMod = 1.0f - (tremoloDepth * (1.0f - lfo));
                clippedSample *= tremoloMod;

                // Update the tremolo phase
                tremoloPhase += 2.0 * juce::MathConstants<double>::pi * tremoloRate * (1.0 / sampleRate);

                // Keep phase in bounds [0, 2π]
                if (tremoloPhase >= 2.0 * juce::MathConstants<double>::pi)
                    tremoloPhase -= 2.0 * juce::MathConstants<double>::pi;
            }

            // Store the current sample as the last sample for the next loop
            lastSample[channel] = clippedSample;

            // Apply the processed sample to the buffer
            channelData[n] = clippedSample;
        }
    }

    // Apply Chorus DSP effect if enabled (tremoloOn could be checked if needed)
    if (tremoloOn)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        chorus.process(juce::dsp::ProcessContextReplacing<float>(block));
    }

    //Clean high frequencies(apply low - pass filter at the end)
    juce::dsp::AudioBlock<float> block(buffer);
    iir.process(juce::dsp::ProcessContextReplacing<float>(block));
}

//==============================================================================
juce::AudioProcessorEditor* DisruptionAudioProcessor::createEditor()
{
    return new PedalComponent(*this, "Disruption", "DIS-1", juce::Colours::midnightblue, { "DRIVE", "LEVEL" });
}

//==============================================================================
bool DisruptionAudioProcessor::hasEditor() const
{
    return true;
}

//==============================================================================
const juce::String DisruptionAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

//==============================================================================
bool DisruptionAudioProcessor::acceptsMidi() const { return false; }
bool DisruptionAudioProcessor::producesMidi() const { return false; }
bool DisruptionAudioProcessor::isMidiEffect() const { return false; }
double DisruptionAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int DisruptionAudioProcessor::getNumPrograms() { return 1; }
int DisruptionAudioProcessor::getCurrentProgram() { return 0; }
void DisruptionAudioProcessor::setCurrentProgram(int index) {}
const juce::String DisruptionAudioProcessor::getProgramName(int index) { return {}; }
void DisruptionAudioProcessor::changeProgramName(int index, const juce::String& newName) {}

//==============================================================================
void DisruptionAudioProcessor::getStateInformation(juce::MemoryBlock& destData) {
    juce::MemoryOutputStream stream(destData, true);
    stream.writeFloat(tremoloRate);
    stream.writeBool(tremoloOn);
}
void DisruptionAudioProcessor::setStateInformation(const void* data, int sizeInBytes) {
    juce::MemoryInputStream stream(data, static_cast<size_t>(sizeInBytes), false);
    tremoloRate = stream.readFloat();
    tremoloOn = stream.readBool();
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DisruptionAudioProcessor();
}

//==============================================================================
// Distortion-related functions
void DisruptionAudioProcessor::updateDistortionCoefficients() {
    R1 = Ts / (2.f * C1);  // Update resistance based on sampling period and capacitance
    updateDistortionGroupedResistances();  // Update grouped resistances
}

void DisruptionAudioProcessor::updateDistortionGroupedResistances() {
    G_distortion = 1.f / (R1 + R3 + Rp);  // Calculate conductance
    Gb = (R3 + Rp) * G_distortion;  // Update Gb based on resistances
    Gi = 1.f + R4 * G_distortion;  // Update Gi
    Gx1 = R1 * R4 * G_distortion;  // Update Gx1
}

float DisruptionAudioProcessor::processDistortionSample(float Vi, float& x1) {

    float Vb = Gb * Vi - R1 * Gb * x1;  // Calculate Vb
    float Vr1 = Vi - Vb;  // Calculate Vr1
    float Vo = Gi * Vi - Gx1 * x1;  // Calculate output voltage

    // Clipping threshold for distortion
    if (Vo > 4.5f) {
        Vo = 4.5f;
    }
    else if (Vo < -4.5f) {
        Vo = -4.5f;
    }

    // Update x1
    x1 = (2.f * Vr1 / R1) - x1;
    return Vo;
}

void DisruptionAudioProcessor::setDistortionKnob(float disKnob) {

    if (disKnob != potDis) {
        potDis = potDis + 0.1f * (disKnob - potDis);  // Adjust the smoothing factor as needed
        Rp = 1.e6 * (1.f - potDis);  // Update Rp based on knob value
        updateDistortionGroupedResistances();  // Update grouped resistances
    }
}

void DisruptionAudioProcessor::prepareDistortion(float newFs) {
    if (newFs != Fs) {
        Fs = newFs;  // Update sample rate
        updateDistortionCoefficients();  // Recalculate coefficients based on new sample rate
    }
}

//==============================================================================
// Clipping-related functions
void DisruptionAudioProcessor::updateClippingCoefficients() {
    Ts = 1.f / Fs;  // Sampling period
    R2 = Ts / (2.f * C2);  // Update R2 based on sampling period and capacitance
    updateClippingGroupedResistances();  // Update grouped resistances for clipping
}

void DisruptionAudioProcessor::updateClippingGroupedResistances() {
    G_clipping = (1 / R5 + 1 / R2);  // Update conductance for clipping
}

float DisruptionAudioProcessor::processClippingSample(float Vi, float& x2) {
    float b = 1.f; // for dampening
    float fd = -Vi / R2 + Is * sinh(Vd / (eta * Vt)) + G_clipping * Vd - x2;
    for (int i = 0; i < 50 && abs(fd) > thr; ++i) {
        float fdd = (Is / (eta * Vt)) * cosh(Vd / (eta * Vt)) + G_clipping;
        float Vnew = Vd - b * fd / fdd;
        float fn = -Vi / R2 + Is * sinh(Vnew / (eta * Vt)) + G_clipping * Vnew - x2;
        if (abs(fn) < abs(fd)) {
            Vd = Vnew;
            b = 1.f;
        }
        else {
            b *= 0.5f;
        }

        fd = -Vi / R2 + Is * sinh(Vd / (eta * Vt)) + G_clipping * Vd - x2;
    }

    x2 = 2 * Vd / R2 - x2;
    return  potLev * Vd;
}

void DisruptionAudioProcessor::setClippingKnob(float levelKnob) {
    if (potLev != levelKnob) {
        potLev = 0.00001f + 0.99998f * levelKnob;  // Scale the level knob value
    }
}

void DisruptionAudioProcessor::prepareClipping(float newFs) {
    if (newFs != Fs) {
        Fs = newFs;  // Update sample rate
        updateClippingCoefficients();  // Recalculate coefficients based on new sample rate
    }
}


void DisruptionAudioProcessor::setTremoloRate(float newRate)
{
    tremoloRate = newRate; // Update the tremolo rate
}
