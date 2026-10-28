# FUTURO — SPRING REVERB

Creative spring-inspired reverb plug-in for macOS Apple Silicon.

## Status

Version 0.9.0 is an engineering build. GitHub Actions builds and validates VST3 and Audio Unit on a native macOS ARM64 runner. The code targets JUCE 9.0.4.

The three modes are **Classicus** (vintage spring harmonics), **Novus** (Voltage Sag: amplitude dip and recovery), and **Somnium** (dark, soft wet-field halo). Character effects work only on the reverberated return: they do not modulate delay time or pitch, and the direct path remains unshifted and unfiltered. The wet field can still be coloured by Tone and each mode’s spring network.

## Build on macOS

Install Apple's Command Line Tools and CMake, then run:

    ./scripts/build_macos.sh

The build script fetches JUCE 9.0.4, compiles VST3 and AU for arm64, and makes an unsigned local test installer in dist.

## GitHub Actions

The workflow at .github/workflows/macos-build.yml builds on GitHub-hosted macOS ARM64 and uploads VST3, AU, .pkg, and SHA-256 checksums as an artifact. Open the Actions tab and download the verified universal macOS artifact from a successful run.

## Validation needed

A successful compilation is not proof of commercial audio quality. The plug-in must still be listened to and validated in the intended DAW, including automation, preset/state recall, tail stability, CPU usage and different sample rates/buffer sizes. The vintage nonlinear stage is not oversampled, so strong drive can still generate aliasing. Objective tests cannot replace listening in the target DAW. The test installer is unsigned and not notarized.

## Installation locations

- /Library/Audio/Plug-Ins/VST3/FUTURO Spring Reverb.vst3
- /Library/Audio/Plug-Ins/Components/FUTURO Spring Reverb.component

Quit the DAW before installing and restart it afterward. Use scripts/uninstall_macos.sh to remove the plug-in bundles.
