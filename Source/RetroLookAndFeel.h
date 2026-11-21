#pragma once
#include <juce_audio_utils/juce_audio_utils.h>

namespace pal
{
inline const juce::Colour bg      { 0xff09090a };
inline const juce::Colour panel   { 0xff131315 };
inline const juce::Colour panelHi { 0xff1d1d20 };
inline const juce::Colour edge    { 0xff2e2e33 };
inline const juce::Colour amber   { 0xffffb23e };
inline const juce::Colour amberDim{ 0xff8a5d17 };
inline const juce::Colour cream   { 0xffe9dcc0 };
inline const juce::Colour red     { 0xffe5482f };
}

inline juce::Font monoFont (float h, bool bold = false)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(), h,
                                          bold ? juce::Font::bold : juce::Font::plain));
}

class RetroLookAndFeel : public juce::LookAndFeel_V4
{
public:
    RetroLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, pal::cream);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxHighlightColourId, pal::amberDim);
        setColour (juce::TextEditor::backgroundColourId, pal::panelHi);
        setColour (juce::TextEditor::textColourId, pal::cream);
        setColour (juce::TextEditor::highlightColourId, pal::amberDim);
        setColour (juce::TextEditor::outlineColourId, pal::amber);
        setColour (juce::TextEditor::focusedOutlineColourId, pal::amber);
        setColour (juce::CaretComponent::caretColourId, pal::amber);
        setColour (juce::ComboBox::backgroundColourId, pal::panelHi);
        setColour (juce::ComboBox::textColourId, pal::cream);
        setColour (juce::ComboBox::outlineColourId, pal::edge);
        setColour (juce::ComboBox::arrowColourId, pal::amber);
        setColour (juce::PopupMenu::backgroundColourId, pal::panel);
        setColour (juce::PopupMenu::textColourId, pal::cream);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, pal::amberDim);
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    }

    juce::Font getLabelFont (juce::Label& l) override
    {
        return monoFont (l.getFont().getHeight() > 13.f ? 12.5f : l.getFont().getHeight());
    }
    juce::Font getComboBoxFont (juce::ComboBox&) override { return monoFont (13.f); }
    juce::Font getPopupMenuFont() override { return monoFont (13.f); }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startA, float endA, juce::Slider&) override
    {
        auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (6.f);
        const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f - 6.f;
        const auto c = b.getCentre();
        const float angle = startA + pos * (endA - startA);

        for (int i = 0; i <= 10; ++i)   // tick scale
        {
            const float a = startA + (endA - startA) * (float) i / 10.f;
            const juce::Point<float> d (std::sin (a), -std::cos (a));
            g.setColour ((float) i / 10.f <= pos + 0.001f ? pal::amber : pal::amberDim.withAlpha (0.6f));
            g.drawLine (juce::Line<float> (c + d * (r + 3.f), c + d * (r + 7.f)), 1.6f);
        }

        g.setColour (juce::Colours::black.withAlpha (0.6f));          // drop shadow
        g.fillEllipse (juce::Rectangle<float> (r * 2.f, r * 2.f).withCentre (c + juce::Point<float> (0.f, 2.f)));

        juce::ColourGradient grad (juce::Colour (0xff4b4b50), c.x - r * 0.5f, c.y - r * 0.7f,
                                   juce::Colour (0xff141416), c.x + r * 0.6f, c.y + r * 0.9f, true);
        g.setGradientFill (grad);
        g.fillEllipse (juce::Rectangle<float> (r * 2.f, r * 2.f).withCentre (c));
        g.setColour (pal::edge.brighter (0.3f));
        g.drawEllipse (juce::Rectangle<float> (r * 2.f, r * 2.f).withCentre (c), 1.2f);

        g.setColour (juce::Colours::white.withAlpha (0.10f));          // knurling rotates with the knob
        for (int i = 0; i < 32; ++i)
        {
            const float a = angle + juce::MathConstants<float>::twoPi * (float) i / 32.f;
            const juce::Point<float> d (std::sin (a), -std::cos (a));
            g.drawLine (juce::Line<float> (c + d * (r - 4.f), c + d * (r - 1.5f)), 1.f);
        }

        const juce::Point<float> d (std::sin (angle), -std::cos (angle));
        g.setColour (pal::amber.withAlpha (0.35f));
        g.drawLine (juce::Line<float> (c + d * (r * 0.30f), c + d * (r * 0.86f)), 5.f);
        g.setColour (pal::amber);
        g.drawLine (juce::Line<float> (c + d * (r * 0.30f), c + d * (r * 0.86f)), 2.4f);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hl, bool) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (0.5f);
        const bool on = b.getToggleState();
        g.setColour (on ? pal::amber.withAlpha (0.14f) : (hl ? pal::panelHi.brighter (0.1f) : pal::panelHi));
        g.fillRoundedRectangle (r, 4.f);
        g.setColour (on ? pal::amber : (hl ? pal::amberDim : pal::edge));
        g.drawRoundedRectangle (r, 4.f, on ? 1.5f : 1.f);
        auto led = juce::Rectangle<float> (9.f, 9.f).withCentre ({ r.getX() + 16.f, r.getCentreY() });
        if (on) { g.setColour (pal::amber.withAlpha (0.30f)); g.fillEllipse (led.expanded (4.f)); g.setColour (pal::amber); }
        else    { g.setColour (pal::edge.brighter (0.25f)); }
        g.fillEllipse (led);
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool) override
    {
        g.setFont (monoFont (13.f, b.getToggleState()));
        g.setColour (b.getToggleState() ? pal::amber : pal::cream.withAlpha (0.8f));
        g.drawText (b.getButtonText(), b.getLocalBounds().withTrimmedLeft (32).withTrimmedRight (6),
                    juce::Justification::centredLeft, true);
    }

    static juce::TextLayout layoutTip (const juce::String& text)
    {
        juce::AttributedString s;
        s.setJustification (juce::Justification::centredLeft);
        s.append (text, monoFont (12.5f), pal::cream);
        juce::TextLayout tl;
        tl.createLayout (s, 330.f);
        return tl;
    }

    juce::Rectangle<int> getTooltipBounds (const juce::String& tip, juce::Point<int> pos, juce::Rectangle<int> area) override
    {
        const auto tl = layoutTip (tip);
        const int w = (int) (tl.getWidth() + 22.f), h = (int) (tl.getHeight() + 16.f);
        return juce::Rectangle<int> (pos.x > area.getCentreX() ? pos.x - (w + 12) : pos.x + 24,
                                     pos.y > area.getCentreY() ? pos.y - (h + 6) : pos.y + 6, w, h).constrainedWithin (area);
    }

    void drawTooltip (juce::Graphics& g, const juce::String& text, int width, int height) override
    {
        juce::Rectangle<float> r (0.f, 0.f, (float) width, (float) height);
        g.setColour (juce::Colour (0xff0d0d0e));
        g.fillRoundedRectangle (r.reduced (0.5f), 4.f);
        g.setColour (pal::amber);
        g.drawRoundedRectangle (r.reduced (1.f), 4.f, 1.2f);
        layoutTip (text).draw (g, r.reduced (11.f, 8.f));
    }
};
