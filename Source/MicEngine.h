#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include "Dsp.h"

class MicEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize, int numChannels);
    void reset();
    int  getLatencySamples() const noexcept { return latencySamples; }
    void process (juce::AudioBuffer<float>& buffer, const r5t::Settings& s);

    std::atomic<float> inRms { 0.f }, inPeak { 0.f }, outRms { 0.f }, outPeak { 0.f };

private:
    static void measure (const juce::AudioBuffer<float>& b, int nCh, std::atomic<float>& rms, std::atomic<float>& peak);

    double sampleRate = 48000.0;
    int latencySamples = 0;

    r5t::Biquad biquads[r5t::kNumSlots];
    r5t::BiquadState eq[2][r5t::kNumSlots];
    r5t::Saturator sat;
    r5t::DeEsser deess;
    double dcX[2] = { 0, 0 }, dcY[2] = { 0, 0 }, dcR = 0.999;

    juce::SmoothedValue<float> trim, outGain;
    juce::dsp::Oversampling<float> oversampler { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true };
    juce::dsp::DryWetMixer<float> mixer { 64 };
};
