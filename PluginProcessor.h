#pragma once

#include <JuceHeader.h>

class PitchDetectorAudioProcessor : public juce::AudioProcessor
{
public:
    PitchDetectorAudioProcessor();
    ~PitchDetectorAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Pitch detection data accessors (thread-safe)
    float getDetectedFrequency() const { return detectedFrequency.load(std::memory_order_relaxed); }
    juce::String getNoteName() const;
    float getCentsOffset() const { return centsOffset.load(std::memory_order_relaxed); }

    // Parameter accessor
    juce::AudioProcessorValueTreeState& getParameters() { return parameters; }

    // Pitch logging
    struct PitchEvent
    {
        double timeInSeconds;
        float frequency;
        int midiNote;
        float velocity;
    };

    void startRecording();
    void stopRecording();
    bool isRecording() const { return recording.load(std::memory_order_relaxed); }
    void clearRecording();
    std::vector<PitchEvent> getPitchLog() const;
    int getLogSize() const { return pitchLog.size(); }

private:
    // Background pitch detection
    void collectSamples(const float* channelData, int numSamples);
    void runPitchDetection();
    float detectPitchYIN(const float* buffer, int numSamples, double sampleRate, float threshold = 0.15f);
    void frequencyToNote(float frequency);

    // Parameters
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float>* bufferSizeParam = nullptr;
    std::atomic<float>* updateRateParam = nullptr;

    // Audio buffers (circular buffer approach)
    std::vector<float> analysisBuffer;
    std::vector<float> processingBuffer;
    std::vector<float> hannWindow;

    std::atomic<int> writePosition{ 0 };
    std::atomic<int> analysisBufferSize{ 4096 }; // Fixed to match constructor
    std::atomic<bool> bufferReady{ false };

    // Detected pitch data (thread-safe atomics)
    std::atomic<float> detectedFrequency{ 0.0f };
    std::atomic<float> centsOffset{ 0.0f };
    juce::String noteName{ "---" };
    juce::CriticalSection noteNameLock;

    std::atomic<double> currentSampleRate{ 48000.0 };

    // DC blocker state
    std::atomic<float> dcBlockerX{ 0.0f };
    std::atomic<float> dcBlockerY{ 0.0f };

    // Timing for analysis
    std::atomic<int> samplesUntilNextAnalysis{ 0 };
    std::atomic<int> currentHopSize{ 4096 };

    // Recording state
    std::atomic<bool> recording{ false };
    std::vector<PitchEvent> pitchLog;
    juce::CriticalSection pitchLogLock;
    double recordingStartTime = 0.0;
    double currentTime = 0.0;
    int sampleCount = 0;

    // Note tracking for MIDI-like behavior
    int lastMidiNote = -1;
    double lastNoteTime = 0.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchDetectorAudioProcessor)
};