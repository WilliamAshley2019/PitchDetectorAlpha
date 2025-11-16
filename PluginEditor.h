#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class PitchDetectorAudioProcessorEditor : public juce::AudioProcessorEditor,
    private juce::Timer
{
public:
    PitchDetectorAudioProcessorEditor(PitchDetectorAudioProcessor&);
    ~PitchDetectorAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    PitchDetectorAudioProcessor& audioProcessor;

    juce::Label noteLabel;
    juce::Label frequencyLabel;
    juce::Label centsLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchDetectorAudioProcessorEditor)
};