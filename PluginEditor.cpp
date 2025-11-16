#include "PluginProcessor.h"
#include "PluginEditor.h"

PitchDetectorAudioProcessorEditor::PitchDetectorAudioProcessorEditor(PitchDetectorAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(600, 550);

    // Note Label
    addAndMakeVisible(noteLabel);
    noteLabel.setFont(juce::Font(60.0f, juce::Font::bold));
    noteLabel.setJustificationType(juce::Justification::centred);
    noteLabel.setText("---", juce::dontSendNotification);
    noteLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    // Frequency Label
    addAndMakeVisible(frequencyLabel);
    frequencyLabel.setFont(juce::Font(20.0f));
    frequencyLabel.setJustificationType(juce::Justification::centred);
    frequencyLabel.setText("0.00 Hz", juce::dontSendNotification);
    frequencyLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);

    // Cents Label
    addAndMakeVisible(centsLabel);
    centsLabel.setFont(juce::Font(18.0f));
    centsLabel.setJustificationType(juce::Justification::centred);
    centsLabel.setText("0 cents", juce::dontSendNotification);
    centsLabel.setColour(juce::Label::textColourId, juce::Colours::yellow);

    // Buffer Size Slider
    addAndMakeVisible(bufferSizeSlider);
    bufferSizeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    bufferSizeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    bufferSizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getParameters(), "bufferSize", bufferSizeSlider);

    addAndMakeVisible(bufferSizeLabel);
    bufferSizeLabel.setText("Buffer Size", juce::dontSendNotification);
    bufferSizeLabel.setJustificationType(juce::Justification::centred);
    bufferSizeLabel.attachToComponent(&bufferSizeSlider, false);

    // Update Rate Combo
    addAndMakeVisible(updateRateCombo);
    updateRateCombo.addItem("2x/sec", 1);
    updateRateCombo.addItem("4x/sec", 2);
    updateRateCombo.addItem("8x/sec", 3);
    updateRateCombo.addItem("12x/sec", 4);
    updateRateCombo.addItem("20x/sec", 5);
    updateRateCombo.addItem("30x/sec", 6);
    updateRateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getParameters(), "updateRate", updateRateCombo);

    addAndMakeVisible(updateRateLabel);
    updateRateLabel.setText("Update Rate", juce::dontSendNotification);
    updateRateLabel.setJustificationType(juce::Justification::centred);
    updateRateLabel.attachToComponent(&updateRateCombo, false);

    // Recording controls
    addAndMakeVisible(recordButton);
    recordButton.setButtonText("Record");
    recordButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red.darker());
    recordButton.onClick = [this]()
        {
            if (audioProcessor.isRecording())
            {
                audioProcessor.stopRecording();
                recordButton.setButtonText("Record");
                recordButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red.darker());
            }
            else
            {
                audioProcessor.startRecording();
                recordButton.setButtonText("Stop");
                recordButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green.darker());
            }
        };

    addAndMakeVisible(clearButton);
    clearButton.setButtonText("Clear Log");
    clearButton.onClick = [this]()
        {
            audioProcessor.clearRecording();
        };

    addAndMakeVisible(recordingStatusLabel);
    recordingStatusLabel.setJustificationType(juce::Justification::centred);
    recordingStatusLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    startTimerHz(30);
}

PitchDetectorAudioProcessorEditor::~PitchDetectorAudioProcessorEditor()
{
}

void PitchDetectorAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a1a));

    // Title bar
    g.setColour(juce::Colour(0xff2a2a2a));
    g.fillRect(0, 0, getWidth(), 30);

    g.setColour(juce::Colours::white);
    g.setFont(16.0f);
    g.drawText("Pitch Detector & Logger", 0, 0, getWidth(), 30,
        juce::Justification::centred, true);

    // Draw tuning indicator
    float cents = audioProcessor.getCentsOffset();
    float freq = audioProcessor.getDetectedFrequency();

    if (freq > 0.0f)
    {
        int centerX = getWidth() / 2;
        int indicatorY = 200;
        int barWidth = 250;

        // Draw center line
        g.setColour(juce::Colours::white);
        g.drawLine(centerX, indicatorY - 20, centerX, indicatorY + 20, 2.0f);

        // Draw range markers
        g.setColour(juce::Colours::grey);
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
        g.drawText("-50", centerX - barWidth / 2 - 25, indicatorY - 6, 20, 12, juce::Justification::right);
        g.drawText("+50", centerX + barWidth / 2 + 5, indicatorY - 6, 20, 12, juce::Justification::left);
    }

    // Draw pitch graph
    auto graphBounds = juce::Rectangle<int>(10, 350, getWidth() - 20, 150);
    drawPitchGraph(g, graphBounds);
}

void PitchDetectorAudioProcessorEditor::drawPitchGraph(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    // Background
    g.setColour(juce::Colour(0xff0a0a0a));
    g.fillRect(bounds);

    g.setColour(juce::Colours::grey);
    g.drawRect(bounds, 1);

    // Get pitch log
    auto pitchLog = audioProcessor.getPitchLog();

    if (pitchLog.empty())
    {
        g.setColour(juce::Colours::grey);
        g.setFont(14.0f);
        g.drawText("Press Record to log pitch data", bounds, juce::Justification::centred);
        return;
    }

    // Draw grid lines (MIDI notes)
    g.setColour(juce::Colour(0xff2a2a2a));
    for (int note = 0; note <= 127; note += 12)
    {
        float y = bounds.getY() + bounds.getHeight() * (1.0f - note / 127.0f);
        g.drawLine(bounds.getX(), y, bounds.getRight(), y, 1.0f);
    }

    // Find time range
    double maxTime = 0.0;
    for (const auto& event : pitchLog)
        maxTime = juce::jmax(maxTime, event.timeInSeconds);

    if (maxTime < 0.1)
        maxTime = 1.0;

    // Draw pitch curve
    juce::Path pitchPath;
    bool firstPoint = true;

    for (const auto& event : pitchLog)
    {
        float x = bounds.getX() + (event.timeInSeconds / maxTime) * bounds.getWidth();
        float y = bounds.getY() + bounds.getHeight() * (1.0f - event.midiNote / 127.0f);

        if (firstPoint)
        {
            pitchPath.startNewSubPath(x, y);
            firstPoint = false;
        }
        else
        {
            pitchPath.lineTo(x, y);
        }
    }

    g.setColour(juce::Colours::cyan);
    g.strokePath(pitchPath, juce::PathStrokeType(2.0f));

    // Draw time markers
    g.setColour(juce::Colours::lightgrey);
    g.setFont(10.0f);
    g.drawText("0s", bounds.getX(), bounds.getBottom() + 2, 30, 12, juce::Justification::left);
    g.drawText(juce::String(maxTime, 1) + "s", bounds.getRight() - 30, bounds.getBottom() + 2, 30, 12, juce::Justification::right);

    // Draw note count
    g.drawText(juce::String(pitchLog.size()) + " events", bounds.getX(), bounds.getY() - 15, 100, 12, juce::Justification::left);
}

void PitchDetectorAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(30); // Title bar
    bounds.reduce(10, 10);

    // Top section - pitch display
    auto topSection = bounds.removeFromTop(220);
    noteLabel.setBounds(topSection.removeFromTop(80));
    frequencyLabel.setBounds(topSection.removeFromTop(30));
    centsLabel.setBounds(topSection.removeFromTop(30));

    bounds.removeFromTop(10); // Spacing

    // Controls section
    auto controlsSection = bounds.removeFromTop(90);

    // Left side - sliders/combos
    auto leftControls = controlsSection.removeFromLeft(getWidth() / 2 - 10);
    bufferSizeLabel.setBounds(leftControls.removeFromTop(20));
    bufferSizeSlider.setBounds(leftControls.removeFromTop(70));

    auto rightControls = controlsSection;
    rightControls.removeFromLeft(10);
    updateRateLabel.setBounds(rightControls.removeFromTop(20));
    updateRateCombo.setBounds(rightControls.removeFromTop(25));

    rightControls.removeFromTop(5);
    auto recordRow = rightControls.removeFromTop(30);
    recordButton.setBounds(recordRow.removeFromLeft(80));
    recordRow.removeFromLeft(10);
    clearButton.setBounds(recordRow.removeFromLeft(80));

    // Status label
    recordingStatusLabel.setBounds(bounds.removeFromTop(20));

    // Graph takes remaining space (handled in paint)
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

    // Update recording status
    if (audioProcessor.isRecording())
    {
        int logSize = audioProcessor.getLogSize();
        recordingStatusLabel.setText("● Recording... (" + juce::String(logSize) + " events)",
            juce::dontSendNotification);
        recordingStatusLabel.setColour(juce::Label::textColourId, juce::Colours::red);
    }
    else
    {
        int logSize = audioProcessor.getLogSize();
        if (logSize > 0)
            recordingStatusLabel.setText("Ready (" + juce::String(logSize) + " events logged)",
                juce::dontSendNotification);
        else
            recordingStatusLabel.setText("Ready", juce::dontSendNotification);
        recordingStatusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    }

    repaint();
}