#include "PluginEditor.h"

namespace
{
struct KnobDef { const char* id; const char* caption; const char* tip; };
const KnobDef kKnobs[9] =
{
 { "trim",      "TRIM",      "INPUT TRIM\nLevel entering the emulation. Aim for peaks around -12 dBFS (INPUT meter near 0 VU) so the preamp stage reacts like a real mic chain.\nDouble-click to reset." },
 { "lowcut",    "LOW CUT",   "LOW CUT\n12 dB/oct high-pass. Removes rumble, handling noise and plosives. Fully left = OFF." },
 { "proximity", "PROXIMITY", "PROXIMITY\nBass build-up when you sing close to a directional mic. Ribbon models react strongest. The NT1 already has its own proximity effect, so use negative values to tighten a boomy close-up take." },
 { "presence",  "PRESENCE",  "PRESENCE\nBoost or cut around the selected mic's own presence peak. + = more clarity and forwardness, - = softer and more distant." },
 { "air",       "AIR",       "AIR\nHigh shelf above 11 kHz. Adds the open, expensive sheen of top-tier condensers. Cut it to calm a bright voice." },
 { "drive",     "DRIVE",     "DRIVE\nPushes the emulated preamp stage (tube, transformer or FET). Low = clean and subtle, high = thicker harmonics and soft compression. 50% = the mic's nominal colour." },
 { "deess",     "DE-ESS",    "DE-ESS\nSplit-band sibilance control above ~5.5 kHz. Bright mics like the C12 and C-800G benefit from a little. 0 = off." },
 { "output",    "OUTPUT",    "OUTPUT\nFinal level. Use it to match loudness between mic models when comparing." },
 { "mix",       "MIX",       "MIX\nBlend between your original NT1 sound (0%) and the emulated mic (100%). Latency-compensated, so parallel blending stays phase-safe." },
};
}

Rode5tuneEditor::Rode5tuneEditor (Rode5tuneProcessor& p)
    : AudioProcessorEditor (&p), proc (p), response (p),
      micAttach (*p.apvts.getParameter ("mic"), [this] (float v)
      {
          currentMic = juce::jlimit (0, r5t::kNumMics - 1, (int) std::lround (v));
          for (int i = 0; i < r5t::kNumMics; ++i)
              if (micButtons[(size_t) i] != nullptr)
                  micButtons[(size_t) i]->setToggleState (i == currentMic, juce::dontSendNotification);
          repaint();
      })
{
    setLookAndFeel (&laf);
    setSize (1000, 700);

    for (int i = 0; i < r5t::kNumMics; ++i)
    {
        const auto& m = r5t::kMics[i];
        auto b = std::make_unique<juce::TextButton> (juce::String::formatted ("%02d  %s", i + 1, m.name));
        b->setClickingTogglesState (false);
        b->setTooltip (juce::String (m.name) + "\n" + m.tagline + "\nBest for: " + m.bestFor + "\nClick to switch the emulation to this microphone.");
        b->onClick = [this, i] { micAttach.setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (*b);
        micButtons[(size_t) i] = std::move (b);
    }

    neutralizeButton.setClickingTogglesState (true);
    neutralizeButton.setTooltip ("NT1 NEUTRALIZER\nApplies a small correction that flattens the NT1 5th Gen's slight presence/air lift, so every emulation starts from a neutral base. Turn it off if you prefer the raw NT1 as the starting point.");
    addAndMakeVisible (neutralizeButton);
    neutAttach = std::make_unique<juce::ButtonParameterAttachment> (*proc.apvts.getParameter ("neutralize"), neutralizeButton);

    preampLabel.setText ("PREAMP STAGE", juce::dontSendNotification);
    preampLabel.setFont (monoFont (11.f, true));
    preampLabel.setColour (juce::Label::textColourId, pal::amber);
    addAndMakeVisible (preampLabel);
    preampBox.addItemList ({ "Native (mic's own)", "Clean", "Transformer", "Tube", "FET" }, 1);
    preampBox.setTooltip ("PREAMP STAGE\nNative = the electronics of the selected mic (tube, transformer or FET). Or force Clean, Transformer, Tube or FET colour on any mic. Intensity is set by DRIVE.");
    addAndMakeVisible (preampBox);
    preampAttach = std::make_unique<juce::ComboBoxParameterAttachment> (*proc.apvts.getParameter ("preamp"), preampBox);

    for (size_t i = 0; i < knobs.size(); ++i) setupKnob (knobs[i], kKnobs[i].id, kKnobs[i].caption, kKnobs[i].tip);

    vuIn.setTooltip ("INPUT METER\nVU-style meter of your NT1 signal. 0 VU = -18 dBFS RMS. The lamp lights if the signal peaks near full scale.");
    vuOut.setTooltip ("OUTPUT METER\nVU-style meter after the emulation. Mic models change loudness, so watch this and use OUTPUT to compensate.");
    response.setTooltip ("TARGET RESPONSE\nLive curve of the tone shaping applied to your NT1 (neutralizer, mic voicing, proximity, presence, air, low cut). It updates as you turn the knobs.");
    addAndMakeVisible (vuIn);
    addAndMakeVisible (vuOut);
    addAndMakeVisible (response);

    micAttach.sendInitialUpdate();
    startTimerHz (30);
}

Rode5tuneEditor::~Rode5tuneEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void Rode5tuneEditor::setupKnob (Knob& k, const char* id, const char* caption, const juce::String& tip)
{
    auto* param = proc.apvts.getParameter (id);
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 76, 18);
    k.slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    k.slider.setTooltip (tip);
    k.slider.setDoubleClickReturnValue (true, param->convertFrom0to1 (param->getDefaultValue()));
    k.attach = std::make_unique<juce::SliderParameterAttachment> (*param, k.slider);
    k.caption.setText (caption, juce::dontSendNotification);
    k.caption.setFont (monoFont (11.f, true));
    k.caption.setJustificationType (juce::Justification::centred);
    k.caption.setColour (juce::Label::textColourId, pal::amber);
    addAndMakeVisible (k.slider);
    addAndMakeVisible (k.caption);
}

void Rode5tuneEditor::timerCallback()
{
    constexpr float dt = 1.f / 30.f;
    vuIn.pushLevel  (proc.engine.inRms.load(),  proc.engine.inPeak.exchange (0.f),  dt);
    vuOut.pushLevel (proc.engine.outRms.load(), proc.engine.outPeak.exchange (0.f), dt);
    response.repaint();
}

void Rode5tuneEditor::resized()
{
    for (int i = 0; i < r5t::kNumMics; ++i) micButtons[(size_t) i]->setBounds (16, 112 + i * 47, 236, 43);
    preampLabel.setBounds (16, 590, 236, 16);
    preampBox.setBounds (16, 608, 236, 28);
    neutralizeButton.setBounds (16, 646, 236, 38);

    response.setBounds (268, 298, 472, 176);
    vuIn.setBounds (752, 86, 232, 204);
    vuOut.setBounds (752, 298, 232, 176);

    const int cellW = 716 / 9;
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const int x = 268 + (int) i * cellW;
        knobs[i].caption.setBounds (x, 520, cellW, 16);
        knobs[i].slider.setBounds (x, 538, cellW, 134);
    }
}

void Rode5tuneEditor::drawScrew (juce::Graphics& g, float x, float y) const
{
    g.setColour (pal::edge.brighter (0.2f));
    g.fillEllipse (x - 3.5f, y - 3.5f, 7.f, 7.f);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawLine (x - 2.f, y + 1.f, x + 2.f, y - 1.f, 1.f);
}

void Rode5tuneEditor::drawPanel (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title) const
{
    auto f = r.toFloat();
    g.setGradientFill (juce::ColourGradient (pal::panelHi, f.getX(), f.getY(), pal::panel, f.getX(), f.getBottom(), false));
    g.fillRoundedRectangle (f, 6.f);
    g.setColour (pal::edge);
    g.drawRoundedRectangle (f.reduced (0.5f), 6.f, 1.f);
    drawScrew (g, f.getX() + 8.f, f.getY() + 8.f);
    drawScrew (g, f.getRight() - 8.f, f.getY() + 8.f);
    drawScrew (g, f.getX() + 8.f, f.getBottom() - 8.f);
    drawScrew (g, f.getRight() - 8.f, f.getBottom() - 8.f);
    g.setColour (pal::amber);
    g.setFont (monoFont (11.f, true));
    g.drawText (title, r.getX() + 22, r.getY() + 6, r.getWidth() - 44, 14, juce::Justification::centredLeft);
}

void Rode5tuneEditor::paint (juce::Graphics& g)
{
    g.fillAll (pal::bg);

    // header with 70s racing stripes
    g.setColour (pal::amber.withAlpha (0.18f));
    g.setFont (monoFont (38.f, true));
    g.drawText ("RODE5TUNE", 19, 9, 400, 46, juce::Justification::centredLeft);
    g.setColour (pal::amber);
    g.drawText ("RODE5TUNE", 16, 6, 400, 46, juce::Justification::centredLeft);
    g.setColour (pal::cream.withAlpha (0.65f));
    g.setFont (monoFont (11.f));
    g.drawText ("MICROPHONE CHARACTER EMULATOR  /  TUNED FOR RODE NT1 5TH GEN", 18, 50, 600, 14, juce::Justification::centredLeft);
    const float sx = 400.f, ex = (float) getWidth() - 16.f;
    g.setColour (pal::red);          g.fillRect (sx, 20.f, ex - sx, 4.f);
    g.setColour (pal::amber);        g.fillRect (sx, 28.f, ex - sx, 4.f);
    g.setColour (pal::cream);        g.fillRect (sx, 36.f, ex - sx, 4.f);
    g.setColour (pal::amberDim);
    g.setFont (monoFont (10.f));
    g.drawText ("v1.0  |  VST3", (int) ex - 140, 46, 140, 14, juce::Justification::centredRight);

    drawPanel (g, { 8, 84, 252, 608 }, "MICROPHONE SELECT");
    drawPanel (g, { 260, 84, 488, 210 }, "MIC PROFILE");
    drawPanel (g, { 260, 296, 488, 182 }, "");
    drawPanel (g, { 260, 482, 732, 210 }, "TONE & PREAMP CONTROLS");

    // mic profile text
    const auto& m = r5t::kMics[(size_t) currentMic];
    const int x = 284, w = 440;
    g.setColour (pal::amber);
    g.setFont (monoFont (24.f, true));
    g.drawText (m.name, x, 106, w, 30, juce::Justification::centredLeft);
    g.setColour (pal::cream.withAlpha (0.7f));
    g.setFont (monoFont (11.f));
    g.drawText (juce::String (m.maker) + "  |  " + m.type + "  |  " + m.pattern, x, 138, w, 14, juce::Justification::centredLeft);
    g.setColour (pal::amber);
    g.drawText (juce::String (m.priceClass) + "  " + m.era, x, 138, w, 14, juce::Justification::centredRight);
    g.setColour (pal::cream);
    g.setFont (monoFont (12.5f, true));
    g.drawText (m.tagline, x, 158, w, 18, juce::Justification::centredLeft);
    g.setFont (monoFont (12.f));
    for (int i = 0; i < 4; ++i)
    {
        g.setColour (pal::amber);
        g.drawText (">", x, 184 + i * 18, 14, 16, juce::Justification::centredLeft);
        g.setColour (pal::cream.withAlpha (0.88f));
        g.drawText (m.traits[i], x + 16, 184 + i * 18, w - 16, 16, juce::Justification::centredLeft);
    }
    g.setColour (pal::amber);
    g.setFont (monoFont (11.f, true));
    g.drawText ("BEST FOR:", x, 262, 80, 16, juce::Justification::centredLeft);
    g.setColour (pal::cream.withAlpha (0.75f));
    g.setFont (monoFont (11.f));
    g.drawText (m.bestFor, x + 82, 262, w - 82, 16, juce::Justification::centredLeft);
}
