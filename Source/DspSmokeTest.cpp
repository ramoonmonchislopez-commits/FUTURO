#include "PluginProcessor.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace
{
    constexpr double testRate = 44100.0;
    constexpr int blockSize = 512;
    constexpr int durationSeconds = 6;

    void setParameter (FuturoSpringAudioProcessor& processor, const char* id, float value)
    {
        if (auto* raw = processor.parameters.getRawParameterValue (id))
            raw->store (value);
    }

    struct RenderResult
    {
        std::vector<float> samples;
        float peak = 0.0f;
        float latePeak = 0.0f;
        bool finite = true;
    };

    RenderResult renderImpulse (int mode, float character = 0.0f, float tone = 0.65f, float intensity = 1.0f, float inputLevel = 1.0f, float mix = 1.0f)
    {
        FuturoSpringAudioProcessor processor;
        setParameter (processor, "mix", mix);
        setParameter (processor, "intensity", intensity);
        setParameter (processor, "saturation", 0.0f);
        setParameter (processor, "character", character);
        setParameter (processor, "tone", tone);
        setParameter (processor, "mode", static_cast<float> (mode));
        processor.prepareToPlay (testRate, blockSize);

        RenderResult result;
        const int totalSamples = static_cast<int> (testRate * durationSeconds);
        result.samples.reserve (static_cast<size_t> (totalSamples));
        juce::AudioBuffer<float> buffer (2, blockSize);
        juce::MidiBuffer midi;

        for (int offset = 0; offset < totalSamples; offset += blockSize)
        {
            buffer.clear();
            if (offset == 0)
            {
                buffer.setSample (0, 0, inputLevel);
                buffer.setSample (1, 0, inputLevel * 0.75f);
            }

            processor.processBlock (buffer, midi);
            const int count = juce::jmin (blockSize, totalSamples - offset);
            for (int i = 0; i < count; ++i)
            {
                const float sample = 0.5f * (buffer.getSample (0, i) + buffer.getSample (1, i));
                if (! std::isfinite (sample))
                    result.finite = false;
                result.peak = juce::jmax (result.peak, std::abs (sample));
                if (offset + i >= totalSamples - static_cast<int> (testRate * 0.5))
                    result.latePeak = juce::jmax (result.latePeak, std::abs (sample));
                result.samples.push_back (sample);
            }
        }

        processor.releaseResources();
        return result;
    }

    bool dryToneFrequencyStable (int mode, float frequency)
    {
        FuturoSpringAudioProcessor processor;
        setParameter (processor, "mix", 0.0f);
        setParameter (processor, "intensity", 0.7f);
        setParameter (processor, "saturation", 0.0f);
        setParameter (processor, "character", 100.0f);
        setParameter (processor, "tone", 1.0f);
        setParameter (processor, "mode", static_cast<float> (mode));
        processor.prepareToPlay (testRate, blockSize);

        juce::AudioBuffer<float> buffer (2, blockSize);
        juce::MidiBuffer midi;
        const int totalSamples = static_cast<int> (testRate);
        double firstCrossing = -1.0;
        double lastCrossing = -1.0;
        int crossings = 0;
        float previous = 0.0f;

        for (int offset = 0; offset < totalSamples; offset += blockSize)
        {
            const int count = juce::jmin (blockSize, totalSamples - offset);
            for (int i = 0; i < count; ++i)
            {
                const int sampleIndex = offset + i;
                const float input = 0.25f * std::sin (juce::MathConstants<float>::twoPi
                    * frequency * static_cast<float> (sampleIndex) / static_cast<float> (testRate));
                buffer.setSample (0, i, input);
                buffer.setSample (1, i, input);
            }
            processor.processBlock (buffer, midi);
            for (int i = 0; i < count; ++i)
            {
                const int sampleIndex = offset + i;
                const float current = 0.5f * (buffer.getSample (0, i) + buffer.getSample (1, i));
                if (sampleIndex > static_cast<int> (0.10 * testRate)
                    && sampleIndex < static_cast<int> (0.90 * testRate)
                    && previous <= 0.0f && current > 0.0f)
                {
                    const double fraction = -static_cast<double> (previous)
                        / static_cast<double> (current - previous);
                    const double crossing = static_cast<double> (sampleIndex - 1) + fraction;
                    if (crossings == 0) firstCrossing = crossing;
                    lastCrossing = crossing;
                    ++crossings;
                }
                previous = current;
            }
        }
        processor.releaseResources();
        if (crossings < 10 || lastCrossing <= firstCrossing) return false;
        const double measured = static_cast<double> (crossings - 1) * testRate / (lastCrossing - firstCrossing);
        const double error = std::abs (measured - static_cast<double> (frequency));
        std::cout << "PASS: mode " << mode << " dry pitch-lock " << measured
                  << " Hz (error " << error << " Hz)\n";
        return error < 0.25;
    }

}

int main()
{
    std::array<RenderResult, 3> renders;
    for (int mode = 0; mode < 3; ++mode)
    {
        renders[static_cast<size_t> (mode)] = renderImpulse (mode);
        const auto& result = renders[static_cast<size_t> (mode)];
        if (! result.finite)
        {
            std::cerr << "FAIL: mode " << mode << " generated NaN/Inf samples\n";
            return 1;
        }
        if (result.peak > 2.0f)
        {
            std::cerr << "FAIL: mode " << mode << " peak exceeded safety ceiling: " << result.peak << "\n";
            return 2;
        }
        if (result.latePeak > 0.001f)
        {
            std::cerr << "FAIL: mode " << mode << " tail did not decay after 6s; late peak="
                      << result.latePeak << "\n";
            return 3;
        }
        std::cout << "PASS: mode " << mode << " peak=" << result.peak
                  << " latePeak=" << result.latePeak << "\n";
    }

    for (int a = 0; a < 3; ++a)
        for (int b = a + 1; b < 3; ++b)
        {
            double diffEnergy = 0.0;
            double refEnergy = 0.0;
            for (size_t i = 0; i < renders[static_cast<size_t> (a)].samples.size(); ++i)
            {
                const double x = renders[static_cast<size_t> (a)].samples[i];
                const double y = renders[static_cast<size_t> (b)].samples[i];
                const double d = x - y;
                diffEnergy += d * d;
                refEnergy += x * x + y * y;
            }
            const double relativeDifference = std::sqrt (diffEnergy / juce::jmax (1.0e-20, refEnergy));
            if (relativeDifference < 0.08)
            {
                std::cerr << "FAIL: modes " << a << " and " << b
                          << " are too similar; relative signal difference=" << relativeDifference << "\n";
                return 4;
            }
            std::cout << "PASS: modes " << a << " vs " << b
                      << " relative difference=" << relativeDifference << "\n";
        }

    // Each mode's CHARACTER knob must audibly alter its wet response.
    for (int mode = 0; mode < 3; ++mode)
    {
        const auto characterMax = renderImpulse (mode, 100.0f);
        double diffEnergy = 0.0, refEnergy = 0.0;
        for (size_t i = 0; i < renders[static_cast<size_t> (mode)].samples.size(); ++i)
        {
            const double a = renders[static_cast<size_t> (mode)].samples[i];
            const double b = characterMax.samples[i];
            const double d = a - b;
            diffEnergy += d * d;
            refEnergy += a * a + b * b;
        }
        const double characterDifference = std::sqrt (diffEnergy / juce::jmax (1.0e-20, refEnergy));
        if (characterDifference < 0.025)
        {
            std::cerr << "FAIL: CHARACTER had too little effect in mode " << mode
                      << "; relative difference=" << characterDifference << "\n";
            return 5;
        }
        std::cout << "PASS: mode " << mode << " CHARACTER difference=" << characterDifference << "\n";
    }

    // TONE extremes must have a meaningful, measurable impact in each mode.
    for (int mode = 0; mode < 3; ++mode)
    {
        const auto dark = renderImpulse (mode, 0.0f, 0.0f);
        const auto bright = renderImpulse (mode, 0.0f, 1.0f);
        double diffEnergy = 0.0, refEnergy = 0.0;
        for (size_t i = 0; i < dark.samples.size(); ++i)
        {
            const double a = dark.samples[i], b = bright.samples[i], d = a - b;
            diffEnergy += d * d;
            refEnergy += a * a + b * b;
        }
        const double toneDifference = std::sqrt (diffEnergy / juce::jmax (1.0e-20, refEnergy));
        if (toneDifference < 0.04)
        {
            std::cerr << "FAIL: TONE had too little effect in mode " << mode
                      << "; relative difference=" << toneDifference << "\n";
            return 6;
        }
        std::cout << "PASS: mode " << mode << " TONE difference=" << toneDifference << "\n";
    }

    // A hot input must remain below digital full scale without using saturation.
    for (int mode = 0; mode < 3; ++mode)
    {
        const auto hot = renderImpulse (mode, 0.0f, 0.65f, 1.0f, 12.0f, 0.0f);
        if (! hot.finite || hot.peak > 0.985f)
        {
            std::cerr << "FAIL: output headroom limiter did not contain peaks in mode " << mode
                      << "; peak=" << hot.peak << "\n";
            return 7;
        }
        std::cout << "PASS: mode " << mode << " clean output ceiling=" << hot.peak << "\n";
    }

    for (int mode = 0; mode < 3; ++mode)
        if (! dryToneFrequencyStable (mode, 997.0f))
        {
            std::cerr << "FAIL: mode " << mode << " altered direct-path pitch at high CHARACTER.\n";
            return 8;
        }

    std::cout << "DSP smoke tests passed: finite signal, bounded peaks, decaying tails, distinct modes, dry pitch lock.\n";
    return 0;
}
