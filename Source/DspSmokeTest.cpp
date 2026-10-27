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

    RenderResult renderImpulse (int mode, float pitch = 0.0f)
    {
        FuturoSpringAudioProcessor processor;
        setParameter (processor, "mix", 1.0f);
        setParameter (processor, "intensity", 1.0f);
        setParameter (processor, "saturation", 0.0f);
        setParameter (processor, "pitch", pitch);
        setParameter (processor, "tone", 0.65f);
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
                buffer.setSample (0, 0, 1.0f);
                buffer.setSample (1, 0, 0.75f);
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

    const auto pitchedFlex = renderImpulse (1, 12.0f);
    double earlyError = 0.0, earlyEnergy = 0.0, tailError = 0.0, tailEnergy = 0.0;
    for (size_t i = 0; i < renders[1].samples.size(); ++i)
    {
        const double a = renders[1].samples[i], b = pitchedFlex.samples[i];
        const double d = a - b;
        if (i < static_cast<size_t> (testRate * 0.020)) { earlyError += d * d; earlyEnergy += a * a + b * b; }
        if (i >= static_cast<size_t> (testRate * 0.150)) { tailError += d * d; tailEnergy += a * a + b * b; }
    }
    if (std::sqrt (earlyError / juce::jmax (1.0e-20, earlyEnergy)) > 1.0e-5)
    {
        std::cerr << "FAIL: pitch changed the first 20ms of the FLEX response\n";
        return 5;
    }
    if (std::sqrt (tailError / juce::jmax (1.0e-20, tailEnergy)) < 0.01)
    {
        std::cerr << "FAIL: pitch did not substantially change the late FLEX tail\n";
        return 6;
    }
    std::cout << "PASS: FLEX pitch leaves the first 20ms stable and changes the late tail\n";

    std::cout << "DSP smoke tests passed: finite signal, bounded peaks, decaying tails, distinct modes.\n";
    return 0;
}
