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
    void drawPitchGraph(juce::Graphics& g, juce::Rectangle<int> bounds);

    PitchDetectorAudioProcessor& audioProcessor;

    juce::Label noteLabel;
    juce::Label frequencyLabel;
    juce::Label centsLabel;

    // Parameter controls
    juce::Slider bufferSizeSlider;
    juce::Label bufferSizeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bufferSizeAttachment;

    juce::ComboBox updateRateCombo;
    juce::Label updateRateLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> updateRateAttachment;

    // Recording controls
    juce::TextButton recordButton;
    juce::TextButton clearButton;
    juce::Label recordingStatusLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchDetectorAudioProcessorEditor)
};