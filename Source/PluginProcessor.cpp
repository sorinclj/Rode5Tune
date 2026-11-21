#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
using FloatP = juce::AudioParameterFloat;

std::unique_ptr<FloatP> makeFloat (const char* id, const char* name, float lo, float hi, float def,
                                   const juce::String& suffix, int decimals, float skew = 1.0f)
{
    auto attrs = juce::AudioParameterFloatAttributes()
        .withStringFromValueFunction ([suffix, decimals, id] (float v, int)
        {
            if (juce::String (id) == "lowcut" && v <= 21.0f) return juce::String ("OFF");
            return juce::String (v, decimals) + suffix;
        })
        .withValueFromStringFunction ([] (const juce::String& t) { return t.getFloatValue(); });
    return std::make_unique<FloatP> (juce::ParameterID { id, 1 }, name,
                                     juce::NormalisableRange<float> (lo, hi, 0.01f, skew), def, attrs);
}
}

juce::AudioProcessorValueTreeState::ParameterLayout Rode5tuneProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    juce::StringArray micNames;
    for (auto& m : r5t::kMics) micNames.add (m.name);

    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "mic", 1 }, "Microphone", micNames, 0));
    p.push_back (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "preamp", 1 }, "Preamp",
                    juce::StringArray { "Native", "Clean", "Transformer", "Tube", "FET" }, 0));
    p.push_back (makeFloat ("trim",      "Input Trim",  -24.f, 24.f,  0.f,   " dB", 1));
    p.push_back (makeFloat ("lowcut",    "Low Cut",      20.f, 250.f, 20.f,  " Hz", 0, 0.5f));
    p.push_back (makeFloat ("proximity", "Proximity",  -100.f, 100.f,  0.f,  " %",  0));
    p.push_back (makeFloat ("presence",  "Presence",     -6.f, 6.f,   0.f,   " dB", 1));
    p.push_back (makeFloat ("air",       "Air",          -6.f, 6.f,   0.f,   " dB", 1));
    p.push_back (makeFloat ("drive",     "Drive",         0.f, 100.f, 50.f,  " %",  0));
    p.push_back (makeFloat ("deess",     "De-Ess",        0.f, 100.f, 0.f,   " %",  0));
    p.push_back (makeFloat ("output",    "Output",      -24.f, 12.f,  0.f,   " dB", 1));
    p.push_back (makeFloat ("mix",       "Mix",           0.f, 100.f, 100.f, " %",  0));
    p.push_back (std::make_unique<juce::AudioParameterBool> (juce::ParameterID { "neutralize", 1 }, "NT1 Neutralizer", true));
    return { p.begin(), p.end() };
}

Rode5tuneProcessor::Rode5tuneProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    pMic = apvts.getRawParameterValue ("mic");       pPreamp = apvts.getRawParameterValue ("preamp");
    pTrim = apvts.getRawParameterValue ("trim");     pLowCut = apvts.getRawParameterValue ("lowcut");
    pProx = apvts.getRawParameterValue ("proximity"); pPres = apvts.getRawParameterValue ("presence");
    pAir = apvts.getRawParameterValue ("air");       pDrive = apvts.getRawParameterValue ("drive");
    pDeess = apvts.getRawParameterValue ("deess");   pOut = apvts.getRawParameterValue ("output");
    pMix = apvts.getRawParameterValue ("mix");       pNeut = apvts.getRawParameterValue ("neutralize");
}

r5t::Settings Rode5tuneProcessor::getSettings() const
{
    r5t::Settings s;
    s.mic        = juce::jlimit (0, r5t::kNumMics - 1, (int) std::lround (pMic->load()));
    s.preamp     = juce::jlimit (0, 4, (int) std::lround (pPreamp->load()));
    s.trimDb     = pTrim->load();
    s.lowCutHz   = pLowCut->load();
    s.proximity  = pProx->load() / 100.f;
    s.presenceDb = pPres->load();
    s.airDb      = pAir->load();
    s.drive      = pDrive->load() / 100.f;
    s.deess      = pDeess->load() / 100.f;
    s.outDb      = pOut->load();
    s.mix        = pMix->load() / 100.f;
    s.neutralize = pNeut->load() > 0.5f;
    return s;
}

void Rode5tuneProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    engine.prepare (sampleRate, samplesPerBlock, juce::jmax (1, getTotalNumInputChannels()));
    setLatencySamples (engine.getLatencySamples());
}

bool Rode5tuneProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto out = l.getMainOutputChannelSet();
    return (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo()) && out == l.getMainInputChannelSet();
}

void Rode5tuneProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, buffer.getNumSamples());
    engine.process (buffer, getSettings());
}

juce::AudioProcessorEditor* Rode5tuneProcessor::createEditor() { return new Rode5tuneEditor (*this); }

void Rode5tuneProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void Rode5tuneProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new Rode5tuneProcessor(); }
