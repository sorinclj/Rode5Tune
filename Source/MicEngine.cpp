#include "MicEngine.h"

void MicEngine::prepare (double fs, int maxBlock, int numChannels)
{
    sampleRate = fs;
    juce::dsp::ProcessSpec spec { fs, (juce::uint32) maxBlock, (juce::uint32) juce::jlimit (1, 2, numChannels) };

    oversampler.initProcessing ((size_t) maxBlock);
    latencySamples = (int) std::lround (oversampler.getLatencyInSamples());

    mixer.prepare (spec);
    mixer.setMixingRule (juce::dsp::DryWetMixingRule::balanced);
    mixer.setWetLatency ((float) latencySamples);

    trim.reset (fs, 0.03);    trim.setCurrentAndTargetValue (1.0f);
    outGain.reset (fs, 0.03); outGain.setCurrentAndTargetValue (1.0f);

    deess.prepare (fs);
    dcR = 1.0 - 2.0 * r5t::kPi * 8.0 / fs;
    reset();
}

void MicEngine::reset()
{
    oversampler.reset();
    mixer.reset();
    for (auto& ch : eq) for (auto& s : ch) s = {};
    deess.reset();
    sat.lp[0] = sat.lp[1] = 0.f;
    dcX[0] = dcX[1] = dcY[0] = dcY[1] = 0.0;
}

void MicEngine::measure (const juce::AudioBuffer<float>& b, int nCh, std::atomic<float>& rms, std::atomic<float>& peak)
{
    float r = 0.f, p = 0.f;
    for (int c = 0; c < nCh; ++c)
    {
        r += b.getRMSLevel (c, 0, b.getNumSamples());
        p = juce::jmax (p, b.getMagnitude (c, 0, b.getNumSamples()));
    }
    rms.store (r / (float) nCh);
    peak.store (juce::jmax (peak.load(), p));
}

void MicEngine::process (juce::AudioBuffer<float>& buffer, const r5t::Settings& s)
{
    const int nCh = juce::jmin (buffer.getNumChannels(), 2);
    const int n   = buffer.getNumSamples();
    if (nCh <= 0 || n <= 0) return;

    measure (buffer, nCh, inRms, inPeak);

    juce::dsp::AudioBlock<float> block (buffer.getArrayOfWritePointers(), (size_t) nCh, (size_t) n);
    mixer.setWetMixProportion (juce::jlimit (0.f, 1.f, s.mix));
    mixer.pushDrySamples (block);

    // 1. input trim
    trim.setTargetValue (juce::Decibels::decibelsToGain (s.trimDb));
    auto** d = buffer.getArrayOfWritePointers();
    for (int i = 0; i < n; ++i)
    {
        const float g = trim.getNextValue();
        for (int c = 0; c < nCh; ++c) d[c][i] *= g;
    }

    // 2. mic voicing (neutralizer -> mic curve -> proximity/presence/air)
    r5t::buildBiquads (s, sampleRate, biquads);
    for (int c = 0; c < nCh; ++c)
        for (int k = 0; k < r5t::kNumSlots; ++k)
            if (biquads[k].active)
                for (int i = 0; i < n; ++i)
                    d[c][i] = (float) r5t::run (biquads[k], eq[c][k], (double) d[c][i]);

    // 3. preamp saturation, 2x oversampled (oversampler always runs to keep latency constant)
    int ptype = r5t::PreampClean; float pd = 0.f;
    r5t::resolvePreamp (s, ptype, pd);
    sat.configure (ptype, pd, sampleRate * 2.0);
    auto up = oversampler.processSamplesUp (block);
    if (sat.active())
        for (int c = 0; c < nCh; ++c)
        {
            float* u = up.getChannelPointer ((size_t) c);
            for (size_t i = 0; i < up.getNumSamples(); ++i) u[i] = sat.process (u[i], c);
        }
    oversampler.processSamplesDown (block);

    // 4. DC block (only after saturation) + de-esser + output gain
    const bool dcOn = sat.active();
    deess.setAmount (s.deess);
    outGain.setTargetValue (juce::Decibels::decibelsToGain (s.outDb));
    for (int i = 0; i < n; ++i)
    {
        const float og = outGain.getNextValue();
        for (int c = 0; c < nCh; ++c)
        {
            float x = d[c][i];
            if (dcOn)
            {
                const double y = (double) x - dcX[c] + dcR * dcY[c];
                dcX[c] = (double) x; dcY[c] = y; x = (float) y;
            }
            if (deess.on) x = deess.process (x, c);
            d[c][i] = x * og;
        }
    }

    // 5. dry/wet (latency compensated)
    mixer.mixWetSamples (block);
    measure (buffer, nCh, outRms, outPeak);
}
