# FUTURO — SPRING REVERB

Creative spring-inspired reverb plug-in for macOS Apple Silicon.

## Status

Version 0.2.0 is an engineering build, not a verified commercial release. GitHub Actions is configured to build it on a native macOS ARM64 runner when a push event is received. The code targets JUCE 9.0.4 and is configured for VST3 and Audio Unit.

The DSP contains a spring-inspired diffusion and resonator feedback network, a crossfaded dual-head pitch shifter on the wet path, smoothed parameters, mono/stereo layouts, and host state serialization. The dry signal is not pitch-shifted.

## Build on macOS

Install Apple's Command Line Tools and CMake, then run:

    ./scripts/build_macos.sh

The build script fetches JUCE 9.0.4, compiles VST3 and AU for arm64, and makes an unsigned local test installer in dist.

## GitHub Actions

The workflow at .github/workflows/macos-build.yml builds on GitHub-hosted macOS ARM64 and uploads VST3, AU, .pkg, and SHA-256 checksums as an artifact. Open the Actions tab and download FUTURO-Spring-Reverb-macOS-arm64 from a successful run.

## Validation needed

A successful compilation is not proof of commercial audio quality. The plug-in must still be listened to and validated in the intended DAW, including automation, preset/state recall, tail stability, pitch artefacts, CPU usage and different sample rates/buffer sizes. Saturation is not yet oversampled, so aliasing can occur at strong drive or with bright material. The test installer is unsigned and not notarized.

## Installation locations

- /Library/Audio/Plug-Ins/VST3/FUTURO Spring Reverb.vst3
- /Library/Audio/Plug-Ins/Components/FUTURO Spring Reverb.component

Quit the DAW before installing and restart it afterward. Use scripts/uninstall_macos.sh to remove the plug-in bundles.
