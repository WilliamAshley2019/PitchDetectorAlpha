#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

PitchDetectorAudioProcessor::PitchDetectorAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    parameters(*this, nullptr, "PARAMETERS",
        {
            std::make_unique<juce::AudioParameterInt>("bufferSize", "Buffer Size", 2048, 16384, 4096),
            std::make_unique<juce::AudioParameterChoice>("updateRate", "Update Rate",
                juce::StringArray{"2x/sec", "4x/sec", "8x/sec", "12x/sec", "20x/sec", "30x/sec"}, 2)
        })
{
    bufferSizeParam = dynamic_cast<std::atomic<float>*>(parameters.getRawParameterValue("bufferSize"));
    updateRateParam = dynamic_cast<std::atomic<float>*>(parameters.getRawParameterValue("updateRate"));

    // Initialize buffers
    int initialSize = 4096;
    analysisBuffer.resize(initialSize, 0.0f);
    processingBuffer.resize(initialSize, 0.0f);
    hannWindow.resize(initialSize);

    // Pre-calculate Hann window
    for (int i = 0; i < initialSize; ++i)
    {
        hannWindow[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / (initialSize - 1)));
    }
}

PitchDetectorAudioProcessor::~PitchDetectorAudioProcessor() = default;

const juce::String PitchDetectorAudioProcessor::getName() const { return JucePlugin_Name; }
bool PitchDetectorAudioProcessor::acceptsMidi() const { return false; }
bool PitchDetectorAudioProcessor::producesMidi() const { return false; }
bool PitchDetectorAudioProcessor::isMidiEffect() const { return false; }
double PitchDetectorAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int PitchDetectorAudioProcessor::getNumPrograms() { return 1; }
int PitchDetectorAudioProcessor::getCurrentProgram() { return 0; }
void PitchDetectorAudioProcessor::setCurrentProgram(int index) {}
const juce::String PitchDetectorAudioProcessor::getProgramName(int index) { return {}; }
void PitchDetectorAudioProcessor::changeProgramName(int index, const juce::String& newName) {}

juce::String PitchDetectorAudioProcessor::getNoteName() const
{
    juce::ScopedLock lock(noteNameLock);
    return noteName;
}

void PitchDetectorAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Store the actual sample rate from the DAW
    currentSampleRate.store(sampleRate, std::memory_order_relaxed);

    // Update buffer size if parameter changed
    int newBufferSize = static_cast<int>(bufferSizeParam->load());
    if (newBufferSize != analysisBufferSize.load())
    {
        analysisBufferSize.store(newBufferSize, std::memory_order_relaxed);
        analysisBuffer.resize(newBufferSize, 0.0f);
        processingBuffer.resize(newBufferSize, 0.0f);
        hannWindow.resize(newBufferSize);

        // Recalculate window
        for (int i = 0; i < newBufferSize; ++i)
        {
            hannWindow[i] = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / (newBufferSize - 1)));
        }
    }

    // Calculate hop size based on update rate parameter
    // Update rates: 2, 4, 8, 12, 20, 30 times per second
    const int updateRates[] = { 2, 4, 8, 12, 20, 30 };
    int updateRateIndex = static_cast<int>(updateRateParam->load());
    int updatesPerSecond = updateRates[updateRateIndex];

    // Calculate samples between updates
    int newHopSize = static_cast<int>(sampleRate / updatesPerSecond);
    currentHopSize.store(newHopSize, std::memory_order_relaxed);

    writePosition.store(0, std::memory_order_relaxed);
    dcBlockerX.store(0.0f, std::memory_order_relaxed);
    dcBlockerY.store(0.0f, std::memory_order_relaxed);
    samplesUntilNextAnalysis.store(0, std::memory_order_relaxed);
    bufferReady.store(false, std::memory_order_relaxed);
}

void PitchDetectorAudioProcessor::releaseResources() {}

bool PitchDetectorAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Support mono or stereo
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo() ||
        layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void PitchDetectorAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    // CRITICAL: This is an ANALYSIS-ONLY plugin
    // Audio passes through 100% unmodified - we just READ from it
    // No processing, no modifications, completely transparent

    // Get input for analysis only (channel 0)
    const auto* channelData = buffer.getReadPointer(0);
    const int numSamples = buffer.getNumSamples();

    // Collect samples for pitch detection (doesn't affect audio output)
    collectSamples(channelData, numSamples);

    // Check if it's time to analyze
    int currentCountdown = samplesUntilNextAnalysis.load(std::memory_order_relaxed);
    currentCountdown -= numSamples;
    samplesUntilNextAnalysis.store(currentCountdown, std::memory_order_relaxed);

    if (currentCountdown <= 0 && bufferReady.load(std::memory_order_relaxed))
    {
        int hopSize = currentHopSize.load(std::memory_order_relaxed);
        samplesUntilNextAnalysis.store(hopSize, std::memory_order_relaxed);
        runPitchDetection();
    }
}

void PitchDetectorAudioProcessor::collectSamples(const float* channelData, int numSamples)
{
    int currentBufferSize = analysisBufferSize.load(std::memory_order_relaxed);
    int pos = writePosition.load(std::memory_order_relaxed);

    float x = dcBlockerX.load(std::memory_order_relaxed);
    float y = dcBlockerY.load(std::memory_order_relaxed);

    for (int i = 0; i < numSamples; ++i)
    {
        // Simple DC blocker
        const float input = channelData[i];
        const float output = input - x + 0.99f * y;
        x = input;
        y = output;

        // Store in circular buffer
        analysisBuffer[pos] = output;
        pos++;

        if (pos >= currentBufferSize)
        {
            pos = 0;
            bufferReady.store(true, std::memory_order_relaxed);
        }
    }

    dcBlockerX.store(x, std::memory_order_relaxed);
    dcBlockerY.store(y, std::memory_order_relaxed);
    writePosition.store(pos, std::memory_order_relaxed);
}

void PitchDetectorAudioProcessor::runPitchDetection()
{
    int currentBufferSize = analysisBufferSize.load(std::memory_order_relaxed);

    // Copy to processing buffer and apply window
    for (int i = 0; i < currentBufferSize; ++i)
    {
        processingBuffer[i] = analysisBuffer[i] * hannWindow[i];
    }

    // Detect pitch
    double sr = currentSampleRate.load(std::memory_order_relaxed);
    const float frequency = detectPitchYIN(processingBuffer.data(), currentBufferSize, sr);

    detectedFrequency.store(frequency, std::memory_order_relaxed);

    if (frequency > 0.0f)
        frequencyToNote(frequency);
    else
    {
        juce::ScopedLock lock(noteNameLock);
        noteName = "---";
        centsOffset.store(0.0f, std::memory_order_relaxed);
    }
}

float PitchDetectorAudioProcessor::detectPitchYIN(const float* buffer, int numSamples, double sampleRate, float threshold)
{
    // Quick RMS check (subsampled)
    float rms = 0.0f;
    for (int i = 0; i < numSamples; i += 4)
        rms += buffer[i] * buffer[i];
    rms = std::sqrt(rms / (numSamples / 4));

    if (rms < 0.005f)
        return 0.0f;

    const int halfSize = numSamples / 2;
    std::vector<float> diff(halfSize);

    // YIN difference function
    for (int tau = 0; tau < halfSize; ++tau)
    {
        float sum = 0.0f;
        for (int i = 0; i < halfSize; ++i)
        {
            const float delta = buffer[i] - buffer[i + tau];
            sum += delta * delta;
        }
        diff[tau] = sum;
    }

    // Cumulative mean normalized difference
    diff[0] = 1.0f;
    float runningSum = 0.0f;

    for (int tau = 1; tau < halfSize; ++tau)
    {
        runningSum += diff[tau];
        if (runningSum > 0.0f)
            diff[tau] *= tau / runningSum;
        else
            diff[tau] = 1.0f;
    }

    // Search range based on musical pitch (50 Hz to 2000 Hz)
    const int minTau = juce::jmax(2, static_cast<int>(sampleRate / 2000.0));
    const int maxTau = juce::jmin(halfSize - 2, static_cast<int>(sampleRate / 50.0));

    int bestTau = 0;

    // Find first local minimum below threshold
    for (int tau = minTau; tau < maxTau; ++tau)
    {
        if (diff[tau] < threshold)
        {
            if (diff[tau] < diff[tau - 1] && diff[tau] < diff[tau + 1])
            {
                bestTau = tau;
                break;
            }
        }
    }

    // Fallback to global minimum
    if (bestTau == 0)
    {
        float minVal = 1.0f;
        for (int tau = minTau; tau < maxTau; ++tau)
        {
            if (diff[tau] < minVal)
            {
                minVal = diff[tau];
                bestTau = tau;
            }
        }
    }

    if (bestTau < 2 || bestTau >= halfSize - 1)
        return 0.0f;

    // Parabolic interpolation
    const float s0 = diff[bestTau - 1];
    const float s1 = diff[bestTau];
    const float s2 = diff[bestTau + 1];

    float refinedTau = static_cast<float>(bestTau);
    const float denom = (s0 - 2.0f * s1 + s2);
    if (std::abs(denom) > 0.0001f)
    {
        const float offset = 0.5f * (s0 - s2) / denom;
        refinedTau += juce::jlimit(-1.0f, 1.0f, offset);
    }

    const float frequency = static_cast<float>(sampleRate / refinedTau);
    return (frequency >= 16.0f && frequency <= 26000.0f) ? frequency : 0.0f;
}

void PitchDetectorAudioProcessor::frequencyToNote(float frequency)
{
    if (frequency < 16.0f || frequency > 26000.0f)
    {
        juce::ScopedLock lock(noteNameLock);
        noteName = "Out of Range";
        centsOffset.store(0.0f, std::memory_order_relaxed);
        return;
    }

    const float midiNote = 12.0f * std::log2(frequency / 440.0f) + 69.0f;
    const int nearestNote = static_cast<int>(std::round(midiNote));
    const float cents = (midiNote - nearestNote) * 100.0f;
    centsOffset.store(cents, std::memory_order_relaxed);

    const char* noteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const char* noteNamesFlat[] = { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };

    const int noteInOctave = nearestNote % 12;
    const int octave = (nearestNote / 12) - 1;

    juce::String newNoteName;
    if (cents < -25.0f && noteInOctave > 0)
        newNoteName = juce::String(noteNamesFlat[noteInOctave]) + juce::String(octave);
    else if (cents > 25.0f && noteInOctave < 11)
        newNoteName = juce::String(noteNames[noteInOctave]) + juce::String(octave);
    else
        newNoteName = juce::String(noteNames[noteInOctave]) + juce::String(octave);

    juce::ScopedLock lock(noteNameLock);
    noteName = newNoteName;
}

bool PitchDetectorAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* PitchDetectorAudioProcessor::createEditor() { return new PitchDetectorAudioProcessorEditor(*this); }

void PitchDetectorAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void PitchDetectorAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PitchDetectorAudioProcessor(); }