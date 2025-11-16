#include "PluginProcessor.h"
#include "PluginEditor.h"

PitchDetectorAudioProcessorEditor::PitchDetectorAudioProcessorEditor(PitchDetectorAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(400, 350);

    // Note Label
    addAndMakeVisible(noteLabel);
    noteLabel.setFont(juce::Font(72.0f, juce::Font::bold));
    noteLabel.setJustificationType(juce::Justification::centred);
    noteLabel.setText("---", juce::dontSendNotification);
    noteLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    // Frequency Label
    addAndMakeVisible(frequencyLabel);
    frequencyLabel.setFont(juce::Font(24.0f));
    frequencyLabel.setJustificationType(juce::Justification::centred);
    frequencyLabel.setText("0.00 Hz", juce::dontSendNotification);
    frequencyLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

    // Cents Label
    addAndMakeVisible(centsLabel);
    centsLabel.setFont(juce::Font(20.0f));
    centsLabel.setJustificationType(juce::Justification::centred);
    centsLabel.setText("0 cents", juce::dontSendNotification);
    centsLabel.setColour(juce::Label::textColourId, juce::Colours::yellow);

    startTimerHz(30); // Update display at 30 Hz
}

PitchDetectorAudioProcessorEditor::~PitchDetectorAudioProcessorEditor()
{
}

void PitchDetectorAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey);

    g.setColour(juce::Colours::grey);
    g.drawRect(getLocalBounds(), 2);

    // Draw title
    g.setColour(juce::Colours::white);
    g.setFont(16.0f);
    g.drawText("Pitch Detector", getLocalBounds().removeFromTop(30),
        juce::Justification::centred, true);

    // Draw tuning indicator
    float cents = audioProcessor.getCentsOffset();
    float freq = audioProcessor.getDetectedFrequency();

    if (freq > 0.0f)
    {
        int centerX = getWidth() / 2;
        int indicatorY = 220;
        int barWidth = 200;

        // Draw center line
        g.setColour(juce::Colours::white);
        g.drawLine(centerX, indicatorY - 20, centerX, indicatorY + 20, 2.0f);

        // Draw range markers
        g.setColour(juce::Colours::darkgrey);
        g.drawLine(centerX - barWidth / 2, indicatorY - 15, centerX - barWidth / 2, indicatorY + 15, 1.0f);
        g.drawLine(centerX + barWidth / 2, indicatorY - 15, centerX + barWidth / 2, indicatorY + 15, 1.0f);

        // Draw cents indicator
        float centsNormalized = juce::jlimit(-50.0f, 50.0f, cents) / 50.0f;
        int indicatorX = centerX + static_cast<int>(centsNormalized * barWidth / 2);

        // Color based on accuracy
        if (std::abs(cents) < 5.0f)
            g.setColour(juce::Colours::green);
        else if (std::abs(cents) < 15.0f)
            g.setColour(juce::Colours::yellow);
        else
            g.setColour(juce::Colours::red);

        g.fillEllipse(indicatorX - 8, indicatorY - 8, 16, 16);

        // Draw cents markers
        g.setColour(juce::Colours::lightgrey);
        g.setFont(12.0f);
        g.drawText("-50", centerX - barWidth / 2 - 20, indicatorY - 6, 20, 12, juce::Justification::right);
        g.drawText("+50", centerX + barWidth / 2, indicatorY - 6, 20, 12, juce::Justification::left);
    }

    // Draw buffer size info
    g.setColour(juce::Colours::lightgrey);
    g.setFont(11.0f);
    auto* bufferParam = audioProcessor.getParameters().getRawParameterValue("bufferSize");
    auto* rateParam = audioProcessor.getParameters().getRawParameterValue("updateRate");
    int bufferSize = static_cast<int>(bufferParam->load());

    const char* rateLabels[] = { "2x/sec", "4x/sec", "8x/sec", "12x/sec", "20x/sec", "30x/sec" };
    int rateIndex = static_cast<int>(rateParam->load());

    g.drawText("Buffer: " + juce::String(bufferSize) + " samples | Rate: " + juce::String(rateLabels[rateIndex]),
        10, getHeight() - 25, 380, 20, juce::Justification::left);
}

void PitchDetectorAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(40); // Space for title

    // Note display
    noteLabel.setBounds(bounds.removeFromTop(100));

    // Frequency display
    frequencyLabel.setBounds(bounds.removeFromTop(40));

    // Cents display
    centsLabel.setBounds(bounds.removeFromTop(35));
}

void PitchDetectorAudioProcessorEditor::timerCallback()
{
    float frequency = audioProcessor.getDetectedFrequency();
    juce::String note = audioProcessor.getNoteName();
    float cents = audioProcessor.getCentsOffset();

    noteLabel.setText(note, juce::dontSendNotification);

    if (frequency > 0.0f)
    {
        frequencyLabel.setText(juce::String(frequency, 2) + " Hz", juce::dontSendNotification);

        juce::String centsText;
        if (cents > 0)
            centsText = "+" + juce::String(cents, 0) + " cents";
        else
            centsText = juce::String(cents, 0) + " cents";

        centsLabel.setText(centsText, juce::dontSendNotification);
    }
    else
    {
        frequencyLabel.setText("0.00 Hz", juce::dontSendNotification);
        centsLabel.setText("0 cents", juce::dontSendNotification);
    }

    repaint();
}