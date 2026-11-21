#pragma once
#include "PluginProcessor.h"
#include "RetroLookAndFeel.h"

// ---- retro VU meter with needle ballistics (0 VU = -18 dBFS RMS) --------------------------
class VuMeter : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit VuMeter (juce::String cap) : caption (std::move (cap)) {}

    void pushLevel (float rms, float peak, float dt)
    {
        const float vu = juce::Decibels::gainToDecibels (rms, -80.f) + 18.f;
        const float target = juce::jlimit (-24.f, 5.f, vu);
        shown += (target - shown) * (1.f - std::exp (-dt / 0.3f));
        if (peak >= 0.98f) clipHold = 45;
        else if (clipHold > 0) --clipHold;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour (pal::panel);
        g.fillRoundedRectangle (b, 6.f);
        g.setColour (pal::edge);
        g.drawRoundedRectangle (b.reduced (0.5f), 6.f, 1.f);

        g.saveState();
        juce::Path clip; clip.addRoundedRectangle (b.reduced (8.f, 8.f), 4.f);
        g.reduceClipRegion (clip);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a2112), b.getCentreX(), b.getBottom(),
                                                 juce::Colour (0xff0d0b07), b.getCentreX(), b.getY(), false));
        g.fillRect (b);

        const float radius = juce::jmin (b.getWidth() * 0.6f, 128.f, b.getHeight() * 0.78f);
        const juce::Point<float> pivot (b.getCentreX(), b.getBottom() - 16.f);
        const float a0 = -0.85f, a1 = 0.85f;
        auto angleFor = [=] (float vu) { return juce::jmap (vu, -20.f, 3.f, a0, a1); };
        auto pt = [&] (float a, float rr) { return pivot + juce::Point<float> (std::sin (a), -std::cos (a)) * rr; };

        juce::Path redZone;                                           // 0..+3 VU
        redZone.addCentredArc (pivot.x, pivot.y, radius * 0.93f, radius * 0.93f, 0.f, angleFor (0.f), a1, true);
        g.setColour (pal::red);
        g.strokePath (redZone, juce::PathStrokeType (3.f));
        juce::Path arc;
        arc.addCentredArc (pivot.x, pivot.y, radius * 0.93f, radius * 0.93f, 0.f, a0, angleFor (0.f), true);
        g.setColour (pal::amber.withAlpha (0.8f));
        g.strokePath (arc, juce::PathStrokeType (3.f));

        g.setFont (monoFont (10.f));
        for (float v : { -20.f, -10.f, -7.f, -5.f, -3.f, 0.f, 3.f })
        {
            const float a = angleFor (v);
            g.setColour (v > 0.f ? pal::red : pal::amber);
            g.drawLine (juce::Line<float> (pt (a, radius * 0.80f), pt (a, radius * 0.93f)), 1.6f);
            g.setColour (pal::cream.withAlpha (0.85f));
            g.drawText (v > 0.f ? "+" + juce::String ((int) v) : juce::String ((int) v),
                        juce::Rectangle<float> (30.f, 12.f).withCentre (pt (a, radius * 0.70f)),
                        juce::Justification::centred);
        }

        g.setColour (pal::cream.withAlpha (0.6f));
        g.setFont (monoFont (11.f, true));
        g.drawText ("VU", juce::Rectangle<float> (40.f, 14.f).withCentre (pivot - juce::Point<float> (0.f, radius * 0.40f)),
                    juce::Justification::centred);

        const float na = angleFor (shown);                            // needle
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.drawLine (juce::Line<float> (pivot + juce::Point<float> (2.f, 2.f), pt (na, radius * 0.95f) + juce::Point<float> (2.f, 2.f)), 2.f);
        g.setColour (pal::amber.brighter (0.3f));
        g.drawLine (juce::Line<float> (pivot, pt (na, radius * 0.95f)), 1.8f);
        g.setColour (pal::edge.brighter (0.4f));
        g.fillEllipse (juce::Rectangle<float> (14.f, 14.f).withCentre (pivot));
        g.restoreState();

        g.setColour (pal::amber);
        g.setFont (monoFont (12.f, true));
        g.drawText (caption, b.reduced (14.f, 10.f).removeFromTop (16.f), juce::Justification::centredLeft);
        auto led = juce::Rectangle<float> (10.f, 10.f).withCentre ({ b.getRight() - 20.f, b.getY() + 18.f });
        g.setColour (clipHold > 0 ? pal::red : pal::edge);
        g.fillEllipse (led);
        g.setColour (pal::cream.withAlpha (0.5f));
        g.setFont (monoFont (9.f));
        g.drawText ("PEAK", led.translated (-34.f, -1.f).withWidth (30.f).withHeight (12.f), juce::Justification::centredRight);
    }

private:
    juce::String caption;
    float shown = -20.f;
    int clipHold = 0;
};

// ---- live frequency-response display (phosphor-green/amber scope look) ------------------
class ResponseView : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit ResponseView (Rode5tuneProcessor& p) : proc (p) {}

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.saveState();
        juce::Path clip; clip.addRoundedRectangle (b, 6.f);
        g.reduceClipRegion (clip);
        g.setColour (juce::Colour (0xff070806));
        g.fillRect (b);

        const juce::Rectangle<float> plot (b.getX() + 40.f, b.getY() + 24.f, b.getWidth() - 54.f, b.getHeight() - 44.f);
        auto fx = [&] (double f) { return plot.getX() + plot.getWidth() * (float) (std::log (f / 20.0) / std::log (1000.0)); };
        auto dy = [&] (double db) { return plot.getCentreY() - (float) db / 12.f * plot.getHeight() * 0.5f; };

        g.setFont (monoFont (9.f));
        for (double f : { 20., 50., 100., 200., 500., 1000., 2000., 5000., 10000., 20000. })
        {
            g.setColour (pal::amber.withAlpha (0.10f));
            g.drawVerticalLine ((int) fx (f), plot.getY(), plot.getBottom());
        }
        const std::pair<double, const char*> lab[] = { { 100., "100" }, { 1000., "1k" }, { 10000., "10k" }, { 20000., "20k" } };
        g.setColour (pal::cream.withAlpha (0.55f));
        for (auto& l : lab)
            g.drawText (l.second, juce::Rectangle<float> (30.f, 12.f).withCentre ({ fx (l.first), plot.getBottom() + 9.f }), juce::Justification::centred);
        for (int db : { -12, -6, 0, 6, 12 })
        {
            g.setColour (pal::amber.withAlpha (db == 0 ? 0.35f : 0.10f));
            g.drawHorizontalLine ((int) dy (db), plot.getX(), plot.getRight());
            g.setColour (pal::cream.withAlpha (0.55f));
            g.drawText ((db > 0 ? "+" : "") + juce::String (db), juce::Rectangle<float> (30.f, 12.f).withCentre ({ plot.getX() - 17.f, dy (db) }),
                        juce::Justification::centredRight);
        }

        const double fs = proc.getCurrentSampleRate() > 0 ? proc.getCurrentSampleRate() : 48000.0;
        r5t::Biquad bq[r5t::kNumSlots];
        r5t::buildBiquads (proc.getSettings(), fs, bq);

        juce::Path p;
        for (float x = plot.getX(); x <= plot.getRight(); x += 2.f)
        {
            const double f = 20.0 * std::pow (1000.0, (x - plot.getX()) / plot.getWidth());
            const float y = dy (juce::jlimit (-12.0, 12.0, r5t::totalResponseDb (bq, f, fs)));
            if (x == plot.getX()) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        juce::Path fill (p);
        fill.lineTo (plot.getRight(), dy (0)); fill.lineTo (plot.getX(), dy (0)); fill.closeSubPath();
        g.setColour (pal::amber.withAlpha (0.10f));
        g.fillPath (fill);
        g.setColour (pal::amber.withAlpha (0.22f));
        g.strokePath (p, juce::PathStrokeType (6.f, juce::PathStrokeType::curved));
        g.setColour (pal::amber.brighter (0.3f));
        g.strokePath (p, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved));

        g.setColour (juce::Colours::black.withAlpha (0.20f));         // CRT scanlines
        for (float y = b.getY(); y < b.getBottom(); y += 3.f) g.fillRect (b.getX(), y, b.getWidth(), 1.f);
        g.restoreState();

        g.setColour (pal::edge);
        g.drawRoundedRectangle (b.reduced (0.5f), 6.f, 1.f);
        g.setColour (pal::amber);
        g.setFont (monoFont (11.f, true));
        g.drawText ("TARGET RESPONSE  (0 dB = flat NT1)", b.reduced (14.f, 6.f).removeFromTop (14.f), juce::Justification::centredLeft);
    }

private:
    Rode5tuneProcessor& proc;
};
