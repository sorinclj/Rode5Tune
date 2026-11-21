#pragma once
#include "PluginProcessor.h"
#include "RetroLookAndFeel.h"
#include "UiComponents.h"

class Rode5tuneEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit Rode5tuneEditor (Rode5tuneProcessor&);
    ~Rode5tuneEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider;
        juce::Label caption;
        std::unique_ptr<juce::SliderParameterAttachment> attach;
    };

    void timerCallback() override;
    void drawPanel (juce::Graphics&, juce::Rectangle<int>, const juce::String& title) const;
    void drawScrew (juce::Graphics&, float x, float y) const;
    void setupKnob (Knob&, const char* id, const char* caption, const juce::String& tip);

    Rode5tuneProcessor& proc;
    RetroLookAndFeel laf;                                   // must outlive all child components
    juce::TooltipWindow tooltipWindow { this, 350 };

    int currentMic = 0;
    std::array<std::unique_ptr<juce::TextButton>, r5t::kNumMics> micButtons;
    juce::TextButton neutralizeButton { "NT1 NEUTRALIZER" };
    juce::ComboBox preampBox;
    juce::Label preampLabel;
    std::array<Knob, 9> knobs;
    VuMeter vuIn { "INPUT" }, vuOut { "OUTPUT" };
    ResponseView response;

    std::unique_ptr<juce::ComboBoxParameterAttachment> preampAttach;
    std::unique_ptr<juce::ButtonParameterAttachment> neutAttach;
    juce::ParameterAttachment micAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Rode5tuneEditor)
};
