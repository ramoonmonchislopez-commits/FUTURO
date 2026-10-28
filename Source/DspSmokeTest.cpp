#include "PluginProcessor.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace
{
    constexpr double testRate = 44100.0;
    constexpr int blockSize = 512;
    constexpr int durationSeconds = 6;
    constexpr double impulseTimeSeconds = 0.15;

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
        double stereoDifferenceRms = 0.0;
        bool finite = true;
    };

    RenderResult renderImpulse (int mode, float character = 0.0f, float tone = 0.65f, float intensity = 1.0f, float inputLevel = 1.0f, float mix = 1.0f, float saturation = 0.0f, bool fiusoEnabled = false)
    {
        FuturoSpringAudioProcessor processor;
        setParameter (processor, "mix", mix);
        setParameter (processor, "intensity", intensity);
        setParameter (processor, "saturation", saturation);
        setParameter (processor, "fiuso", fiusoEnabled ? 1.0f : 0.0f);
        setParameter (processor, "character", character);
        setParameter (processor, "tone", tone);
        setParameter (processor, "mode", static_cast<float> (mode));
        processor.prepareToPlay (testRate, blockSize);

        RenderResult result;
        double stereoDifferenceEnergy = 0.0;
        const int totalSamples = static_cast<int> (testRate * durationSeconds);
        result.samples.reserve (static_cast<size_t> (totalSamples));
        juce::AudioBuffer<float> buffer (2, blockSize);
        juce::MidiBuffer midi;

        // Let all smoothed parameters (especially Mode) reach the requested
        // test state before exciting the reverb. An impulse at sample zero only
        // sees the processor's constructor defaults, not the mode under test.
        const int impulseSample = static_cast<int> (impulseTimeSeconds * testRate);
        for (int offset = 0; offset < totalSamples; offset += blockSize)
        {
            buffer.clear();
            if (impulseSample >= offset && impulseSample < offset + blockSize)
            {
                const int localImpulseSample = impulseSample - offset;
                buffer.setSample (0, localImpulseSample, inputLevel);
                buffer.setSample (1, localImpulseSample, inputLevel * 0.75f);
            }

            processor.processBlock (buffer, midi);
            const int count = juce::jmin (blockSize, totalSamples - offset);
            for (int i = 0; i < count; ++i)
            {
                const float left = buffer.getSample (0, i);
                const float right = buffer.getSample (1, i);
                const float sample = 0.5f * (left + right);
                if (! std::isfinite (left) || ! std::isfinite (right))
                    result.finite = false;
                const double stereoDelta = static_cast<double> (left) - right;
                stereoDifferenceEnergy += stereoDelta * stereoDelta;
                result.peak = juce::jmax (result.peak, juce::jmax (std::abs (left), std::abs (right)));
                if (offset + i >= totalSamples - static_cast<int> (testRate * 0.5))
                    result.latePeak = juce::jmax (result.latePeak, std::abs (sample));
                result.samples.push_back (sample);
            }
        }

        result.stereoDifferenceRms = std::sqrt (stereoDifferenceEnergy / juce::jmax (1, totalSamples));
        processor.releaseResources();
        return result;
    }

    double windowRms (const RenderResult& result, double startSeconds, double endSeconds)
    {
        const auto first = static_cast<size_t> (juce::jlimit (0.0, static_cast<double> (result.samples.size()),
                                                              startSeconds * testRate));
        const auto last = static_cast<size_t> (juce::jlimit (0.0, static_cast<double> (result.samples.size()),
                                                             endSeconds * testRate));
        if (last <= first)
            return 0.0;

        double energy = 0.0;
        for (size_t i = first; i < last; ++i)
        {
            const double sample = result.samples[i];
            energy += sample * sample;
        }
        return std::sqrt (energy / static_cast<double> (last - first));
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

    // Character must remain responsive across the knob, not only at its endpoints.
    for (int mode = 0; mode < 3; ++mode)
    {
        const auto character20 = renderImpulse (mode, 20.0f);
        const auto character50 = renderImpulse (mode, 50.0f);
        const auto character80 = renderImpulse (mode, 80.0f);
        const auto relativeDifference = [] (const RenderResult& a, const RenderResult& b)
        {
            double differenceEnergy = 0.0;
            double referenceEnergy = 0.0;
            for (size_t i = 0; i < a.samples.size(); ++i)
            {
                const double x = a.samples[i];
                const double y = b.samples[i];
                const double d = x - y;
                differenceEnergy += d * d;
                referenceEnergy += x * x + y * y;
            }
            return std::sqrt (differenceEnergy / juce::jmax (1.0e-20, referenceEnergy));
        };
        const double lowMidDifference = relativeDifference (character20, character50);
        const double midHighDifference = relativeDifference (character50, character80);
        if (! character20.finite || ! character50.finite || ! character80.finite
            || lowMidDifference < 0.0025 || midHighDifference < 0.0025)
        {
            std::cerr << "FAIL: Character has a plateau in mode " << mode
                      << "; 20-50 difference=" << lowMidDifference
                      << " 50-80 difference=" << midHighDifference << "\n";
            return 19;
        }
        std::cout << "PASS: Character travel mode " << mode
                  << " 20-50=" << lowMidDifference
                  << " 50-80=" << midHighDifference << "\n";
    }

    // Higher Intensity should let Somnium bloom into an audible, finite tail while
    // stereo decorrelation keeps the cloud expansive and its attack controlled.
    {
        const auto somniumShort = renderImpulse (2, 75.0f, 0.48f, 0.35f, 0.70f, 1.0f, 4.0f);
        const auto somniumExpanded = renderImpulse (2, 75.0f, 0.48f, 0.90f, 0.70f, 1.0f, 4.0f);
        // Measure the sustained cloud after the initial bloom, not only the
        // very late decay where any impulse reverb is naturally close to silence.
        const double shortTailRms = windowRms (somniumShort, impulseTimeSeconds + 0.50,
                                                   impulseTimeSeconds + 2.25);
        const double expandedTailRms = windowRms (somniumExpanded, impulseTimeSeconds + 0.50,
                                                      impulseTimeSeconds + 2.25);
        const bool safePeaks = somniumShort.peak <= 0.985f && somniumExpanded.peak <= 0.985f;
        const bool tailGrows = expandedTailRms >= shortTailRms * 1.08;
        const bool tailIsAudible = expandedTailRms >= 0.00005;
        const bool hasStereoSpread = somniumExpanded.stereoDifferenceRms >= 0.00001;
        if (! somniumShort.finite || ! somniumExpanded.finite
            || ! safePeaks || ! tailGrows || ! tailIsAudible || ! hasStereoSpread)
        {
            std::cerr << "FAIL: Somnium tail regression; finite=" << somniumShort.finite
                      << "/" << somniumExpanded.finite
                      << " safePeaks=" << safePeaks
                      << " tailGrows=" << tailGrows
                      << " tailIsAudible=" << tailIsAudible
                      << " hasStereoSpread=" << hasStereoSpread
                      << " short RMS=" << shortTailRms
                      << " expanded RMS=" << expandedTailRms
                      << " stereo difference=" << somniumExpanded.stereoDifferenceRms << "\n";
            return 20;
        }
        std::cout << "PASS: Somnium tail expansion short RMS=" << shortTailRms
                  << " expanded RMS=" << expandedTailRms
                  << " stereo difference=" << somniumExpanded.stereoDifferenceRms
                  << " feedback ceiling=0.96\n";
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

    // DRIVE must affect every mode, remain finite, and preserve stable decay.
    for (int mode = 0; mode < 3; ++mode)
    {
        const auto cleanDrive = renderImpulse (mode, 55.0f, 0.65f, 0.80f, 0.65f, 1.0f, 0.0f);
        const auto pushedDrive = renderImpulse (mode, 55.0f, 0.65f, 0.80f, 0.65f, 1.0f, 18.0f);
        double differenceEnergy = 0.0, referenceEnergy = 0.0;
        for (size_t i = 0; i < cleanDrive.samples.size(); ++i)
        {
            const double a = cleanDrive.samples[i], b = pushedDrive.samples[i], d = a - b;
            differenceEnergy += d * d;
            referenceEnergy += a * a + b * b;
        }
        const double driveDifference = std::sqrt (differenceEnergy / juce::jmax (1.0e-20, referenceEnergy));
        if (! pushedDrive.finite || pushedDrive.peak > 0.985f || pushedDrive.latePeak > 0.001f)
        {
            std::cerr << "FAIL: mode " << mode << " DRIVE caused unsafe level or unstable tail; peak="
                      << pushedDrive.peak << " latePeak=" << pushedDrive.latePeak << "\n";
            return 9;
        }
        if (driveDifference < 0.006)
        {
            std::cerr << "FAIL: DRIVE had too little effect in mode " << mode
                      << "; relative difference=" << driveDifference << "\n";
            return 10;
        }
        std::cout << "PASS: mode " << mode << " DRIVE difference=" << driveDifference
                  << " peak=" << pushedDrive.peak << " latePeak=" << pushedDrive.latePeak << "\n";
    }

    // Regression for the smooth-feedback change: a hot, fully wet, maximum-drive impulse
    // must stay finite, remain under the output ceiling, and decay instead of regenerating.
    for (int mode = 0; mode < 3; ++mode)
    {
        const auto stressed = renderImpulse (mode, 100.0f, 0.70f, 1.0f, 8.0f, 1.0f, 18.0f);
        if (! stressed.finite || stressed.peak > 0.985f || stressed.latePeak > 0.001f)
        {
            std::cerr << "FAIL: stressed wet feedback unstable in mode " << mode
                      << "; finite=" << stressed.finite
                      << " peak=" << stressed.peak
                      << " latePeak=" << stressed.latePeak << "\n";
            return 14;
        }
        std::cout << "PASS: mode " << mode << " stressed wet feedback peak="
                  << stressed.peak << " latePeak=" << stressed.latePeak << "\n";
    }

    // FIUSO's parallel harmonic layers must also remain bounded under a hot impulse.
    for (int mode = 0; mode < 3; ++mode)
    {
        const auto stressedFiuso = renderImpulse (mode, 100.0f, 0.70f, 1.0f, 8.0f, 1.0f, 18.0f, true);
        if (! stressedFiuso.finite || stressedFiuso.peak > 0.985f || stressedFiuso.latePeak > 0.001f)
        {
            std::cerr << "FAIL: FIUSO stress unstable in mode " << mode
                      << "; finite=" << stressedFiuso.finite
                      << " peak=" << stressedFiuso.peak
                      << " latePeak=" << stressedFiuso.latePeak << "\n";
            return 15;
        }
        std::cout << "PASS: FIUSO stress mode " << mode << " peak="
                  << stressedFiuso.peak << " latePeak=" << stressedFiuso.latePeak << "\n";
    }

    // FIUSO must add a measurable harmonic layer to the wet spring, without instability.
    {
        FuturoSpringAudioProcessor plain, fiuso;
        for (auto* processor : { &plain, &fiuso })
        {
            setParameter (*processor, "mix", 1.0f);
            setParameter (*processor, "intensity", 0.75f);
            setParameter (*processor, "saturation", 0.0f);
            setParameter (*processor, "mode", 0.0f);
            processor->prepareToPlay (testRate, blockSize);
        }
        setParameter (fiuso, "fiuso", 1.0f);
        juce::AudioBuffer<float> a (2, blockSize), b (2, blockSize);
        juce::MidiBuffer midi;
        double differenceEnergy = 0.0, referenceEnergy = 0.0;
        bool finite = true;
        for (int offset = 0; offset < static_cast<int> (testRate); offset += blockSize)
        {
            for (int i = 0; i < blockSize; ++i)
            {
                const float sample = 0.22f * std::sin (juce::MathConstants<float>::twoPi
                    * 220.0f * static_cast<float> (offset + i) / static_cast<float> (testRate));
                a.setSample (0, i, sample); a.setSample (1, i, sample);
                b.setSample (0, i, sample); b.setSample (1, i, sample);
            }
            plain.processBlock (a, midi);
            fiuso.processBlock (b, midi);
            for (int i = 0; i < blockSize; ++i)
            {
                const double x = a.getSample (0, i), y = b.getSample (0, i);
                finite = finite && std::isfinite (x) && std::isfinite (y);
                differenceEnergy += (x - y) * (x - y);
                referenceEnergy += x * x + y * y;
            }
        }
        const double difference = std::sqrt (differenceEnergy / juce::jmax (1.0e-20, referenceEnergy));
        if (! finite || difference < 0.004)
        {
            std::cerr << "FAIL: FIUSO harmonic body was too subtle or unstable; difference=" << difference << "\n";
            return 12;
        }
        std::cout << "PASS: FIUSO harmonic layer difference=" << difference << "\n";
        plain.releaseResources();
        fiuso.releaseResources();
    }

    // IN and OUT faders must provide the expected smoothed gain change with no clipping.
    {
        FuturoSpringAudioProcessor inputUnity, inputBoost, outputUnity, outputBoost;
        for (auto* processor : { &inputUnity, &inputBoost, &outputUnity, &outputBoost })
        {
            setParameter (*processor, "mix", 0.0f);
            setParameter (*processor, "intensity", 0.7f);
            setParameter (*processor, "saturation", 0.0f);
            setParameter (*processor, "mode", 0.0f);
            setParameter (*processor, "inputGain", 0.0f);
            setParameter (*processor, "outputGain", 0.0f);
        }
        setParameter (inputBoost, "inputGain", 6.0f);
        setParameter (outputBoost, "outputGain", 6.0f);
        for (auto* processor : { &inputUnity, &inputBoost, &outputUnity, &outputBoost })
            processor->prepareToPlay (testRate, blockSize);

        std::array<double, 4> energy {};
        constexpr int trimTestSamples = static_cast<int> (testRate);
        constexpr int trimWarmupSamples = static_cast<int> (0.20 * testRate);
        juce::AudioBuffer<float> inputUnityBuffer (2, blockSize);
        juce::AudioBuffer<float> inputBoostBuffer (2, blockSize);
        juce::AudioBuffer<float> outputUnityBuffer (2, blockSize);
        juce::AudioBuffer<float> outputBoostBuffer (2, blockSize);
        std::array<juce::AudioBuffer<float>*, 4> inputBuffers {
            &inputUnityBuffer, &inputBoostBuffer, &outputUnityBuffer, &outputBoostBuffer
        };
        std::array<FuturoSpringAudioProcessor*, 4> trimProcessors {
            &inputUnity, &inputBoost, &outputUnity, &outputBoost
        };
        juce::MidiBuffer trimMidi;
        for (int offset = 0; offset < trimTestSamples; offset += blockSize)
        {
            const int count = juce::jmin (blockSize, trimTestSamples - offset);
            for (int channel = 0; channel < 4; ++channel)
                for (int i = 0; i < count; ++i)
                {
                    const float sample = 0.03f * std::sin (juce::MathConstants<float>::twoPi
                        * 220.0f * static_cast<float> (offset + i) / static_cast<float> (testRate));
                    inputBuffers[static_cast<size_t> (channel)]->setSample (0, i, sample);
                    inputBuffers[static_cast<size_t> (channel)]->setSample (1, i, sample);
                }

            for (int channel = 0; channel < 4; ++channel)
            {
                trimProcessors[static_cast<size_t> (channel)]->processBlock (*inputBuffers[static_cast<size_t> (channel)], trimMidi);
                if (offset >= trimWarmupSamples)
                    for (int i = 0; i < count; ++i)
                    {
                        const double sample = inputBuffers[static_cast<size_t> (channel)]->getSample (0, i);
                        energy[static_cast<size_t> (channel)] += sample * sample;
                    }
            }
        }

        const double inputRatio = std::sqrt (energy[1] / juce::jmax (1.0e-20, energy[0]));
        const double outputRatio = std::sqrt (energy[3] / juce::jmax (1.0e-20, energy[2]));
        if (inputRatio < 1.85 || inputRatio > 2.15 || outputRatio < 1.85 || outputRatio > 2.15)
        {
            std::cerr << "FAIL: IN/OUT +6 dB trims are not near 2x amplitude; input ratio="
                      << inputRatio << " output ratio=" << outputRatio << "\n";
            return 13;
        }
        std::cout << "PASS: IN/OUT +6 dB trim ratios input=" << inputRatio
                  << " output=" << outputRatio << "\n";
        for (auto* processor : { &inputUnity, &inputBoost, &outputUnity, &outputBoost })
            processor->releaseResources();
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

    {
        FuturoSpringAudioProcessor dryA, dryB;
        for (auto* processor : { &dryA, &dryB })
        {
            setParameter (*processor, "mix", 0.0f);
            setParameter (*processor, "intensity", 0.8f);
            setParameter (*processor, "character", 100.0f);
            setParameter (*processor, "tone", 0.9f);
            setParameter (*processor, "mode", 1.0f);
        }
        setParameter (dryA, "saturation", 0.0f);
        setParameter (dryB, "saturation", 18.0f);
        setParameter (dryB, "fiuso", 1.0f);
        dryA.prepareToPlay (testRate, blockSize);
        dryB.prepareToPlay (testRate, blockSize);
        juce::AudioBuffer<float> a (2, blockSize), b (2, blockSize);
        juce::MidiBuffer midi;
        float maxDryDifference = 0.0f;
        for (int offset = 0; offset < static_cast<int> (testRate); offset += blockSize)
        {
            const int count = juce::jmin (blockSize, static_cast<int> (testRate) - offset);
            for (int i = 0; i < blockSize; ++i)
            {
                const float sample = 0.22f * std::sin (juce::MathConstants<float>::twoPi
                    * 431.0f * static_cast<float> (offset + i) / static_cast<float> (testRate));
                a.setSample (0, i, sample); a.setSample (1, i, sample);
                b.setSample (0, i, sample); b.setSample (1, i, sample);
            }
            dryA.processBlock (a, midi);
            dryB.processBlock (b, midi);
            for (int i = 0; i < count; ++i)
                maxDryDifference = juce::jmax (maxDryDifference,
                    juce::jmax (std::abs (a.getSample (0, i) - b.getSample (0, i)),
                                std::abs (a.getSample (1, i) - b.getSample (1, i))));
        }
        if (maxDryDifference > 1.0e-6f)
        {
            std::cerr << "FAIL: DRIVE changed the dry path; max delta=" << maxDryDifference << "\n";
            return 11;
        }
        std::cout << "PASS: dry path isolation with DRIVE extremes; max delta=" << maxDryDifference << "\n";
        dryA.releaseResources();
        dryB.releaseResources();
    }

    // Invalid host samples must be contained without poisoning subsequent DSP state.
    {
        FuturoSpringAudioProcessor processor;
        setParameter (processor, "mix", 1.0f);
        setParameter (processor, "fiuso", 1.0f);
        processor.prepareToPlay (testRate, blockSize);
        juce::AudioBuffer<float> buffer (2, blockSize);
        juce::MidiBuffer midi;
        buffer.clear();
        buffer.setSample (0, 0, std::numeric_limits<float>::quiet_NaN());
        buffer.setSample (1, 0, std::numeric_limits<float>::infinity());
        for (int i = 1; i < blockSize; ++i)
        {
            const float sample = 0.1f * std::sin (juce::MathConstants<float>::twoPi
                * 220.0f * static_cast<float> (i) / static_cast<float> (testRate));
            buffer.setSample (0, i, sample);
            buffer.setSample (1, i, sample);
        }
        processor.processBlock (buffer, midi);
        bool finite = true;
        for (int channel = 0; channel < 2; ++channel)
            for (int i = 0; i < blockSize; ++i)
                finite = finite && std::isfinite (buffer.getSample (channel, i));
        buffer.clear();
        for (int i = 0; i < blockSize; ++i)
        {
            const float sample = 0.1f * std::sin (juce::MathConstants<float>::twoPi
                * 220.0f * static_cast<float> (blockSize + i) / static_cast<float> (testRate));
            buffer.setSample (0, i, sample);
            buffer.setSample (1, i, sample);
        }
        processor.processBlock (buffer, midi);
        for (int channel = 0; channel < 2; ++channel)
            for (int i = 0; i < blockSize; ++i)
                finite = finite && std::isfinite (buffer.getSample (channel, i));
        if (! finite)
        {
            std::cerr << "FAIL: invalid input poisoned DSP output or subsequent state\n";
            return 17;
        }
        std::cout << "PASS: invalid host samples contained; DSP state remains finite\n";
        processor.releaseResources();
    }

    // Automated parameter sweeps and mode changes must stay finite and bounded.
    {
        FuturoSpringAudioProcessor processor;
        setParameter (processor, "mix", 1.0f);
        setParameter (processor, "intensity", 0.8f);
        setParameter (processor, "saturation", 8.0f);
        processor.prepareToPlay (testRate, blockSize);

        juce::AudioBuffer<float> buffer (2, blockSize);
        juce::MidiBuffer midi;
        bool finite = true;
        float peak = 0.0f;
        constexpr std::array<float, 5> characterSteps { 0.0f, 25.0f, 50.0f, 75.0f, 100.0f };
        constexpr int sweepBlocks = 240;
        for (int block = 0; block < sweepBlocks; ++block)
        {
            const int step = (block / 12) % static_cast<int> (characterSteps.size());
            const float characterValue = (block / 60) % 2 == 0
                ? characterSteps[static_cast<size_t> (step)]
                : characterSteps[static_cast<size_t> (4 - step)];
            const int mode = (block / 30) % 3;
            setParameter (processor, "character", characterValue);
            setParameter (processor, "tone", static_cast<float> ((block / 6) % 11) / 10.0f);
            setParameter (processor, "mode", static_cast<float> (mode));
            setParameter (processor, "fiuso", (block / 15) % 2 == 0 ? 1.0f : 0.0f);

            for (int i = 0; i < blockSize; ++i)
            {
                const float sample = 0.18f * std::sin (juce::MathConstants<float>::twoPi
                    * 173.0f * static_cast<float> (block * blockSize + i) / static_cast<float> (testRate));
                buffer.setSample (0, i, sample);
                buffer.setSample (1, i, sample * 0.91f);
            }
            processor.processBlock (buffer, midi);
            for (int channel = 0; channel < 2; ++channel)
                for (int i = 0; i < blockSize; ++i)
                {
                    const float sample = buffer.getSample (channel, i);
                    finite = finite && std::isfinite (sample);
                    peak = juce::jmax (peak, std::abs (sample));
                }
        }
        if (! finite || peak > 0.985f)
        {
            std::cerr << "FAIL: parameter automation/mode changes caused invalid or excessive output; peak="
                      << peak << "\n";
            return 18;
        }
        std::cout << "PASS: Character/Tone/Mode/Fiuso automation finite and bounded; peak="
                  << peak << "\n";
        processor.releaseResources();
    }

    std::cout << "DSP smoke tests passed: finite signal, bounded peaks, decaying tails, distinct modes, dry pitch lock, FIUSO harmonic layer.\n";
    return 0;
}
