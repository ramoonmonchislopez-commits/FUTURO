# SPIRITUS — SPRING REVERB

Expressive spring reverb for macOS (Apple Silicon + Intel). Classicus brings a stronger vintage spring voice, Novus has a more audible voltage-sag dip and recovery, and Somnium keeps a dark, enveloping halo. Intensity reaches longer decay and more body/presence on the wet return, while the direct path remains uncoloured.

## Status

Version 0.12.1 is an engineering build. GitHub Actions builds and validates VST3 and Audio Unit on a native macOS ARM64 runner. The code targets JUCE 9.0.4.

The three modes are **Classicus / Spring** (rounded, zero-centred vintage drive), **Novus / Sag** (power-stage compression plus Voltage Sag: amplitude dip and recovery), and **Somnium / Halo** (soft peak rounding with a dark, wet-field halo). Drive uses a different smooth, bounded saturation curve per mode and remains wet-only. The mode-dependent Character control is explicitly labelled Spring, Sag, or Halo. The small **IN** and **OUT** faders provide smoothed input/output trim. The central **Fiuso** button adds a DC-blocked octave-up harmonic series and related even harmonics to the spring excitation; the on/off state is smoothed and the dry path stays free of the generated layer. Mode-specific colours are crossfaded continuously; the dry path is not pitch-modulated or time-shifted. Intensity controls decay and wet presence, while Tone shapes the wet return.

## Build on macOS

Install Apple's Command Line Tools and CMake, then run:

    ./scripts/build_macos.sh

The build script fetches JUCE 9.0.4, compiles VST3 and AU for arm64, and makes an unsigned local test installer in dist.

## GitHub Actions

The workflow at .github/workflows/macos-build.yml builds on GitHub-hosted macOS ARM64 and uploads VST3, AU, .pkg, and SHA-256 checksums as an artifact. Open the Actions tab and download the verified universal macOS artifact from a successful run.

## DSP validation

The automated DSP suite covers finite output, stable decay, mode differentiation, Character and Tone response, mode-specific Drive response, the Fiuso harmonic layer, IN/OUT gain-trim response, output headroom, dry-path pitch lock, and dry-path isolation from Drive.

## Validation needed

A successful compilation is not proof of commercial audio quality. The plug-in must still be listened to and validated in the intended DAW, including automation, preset/state recall, tail stability, CPU usage and different sample rates/buffer sizes. The nonlinear stages are not oversampled, so extreme Drive or Fiuso settings can still generate some aliasing. Fiuso shapes the spring excitation with harmonic content; it is not a pitch-tracking polyphonic harmonizer. Objective tests cannot replace listening in the target DAW. The test installer is unsigned and not notarized.

## Installation locations

- /Library/Audio/Plug-Ins/VST3/SPIRITUS Spring Reverb.vst3
- /Library/Audio/Plug-Ins/Components/SPIRITUS Spring Reverb.component

Quit the DAW before installing and restart it afterward. Use scripts/uninstall_macos.sh to remove the plug-in bundles.
