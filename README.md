# SPIRITUS — SPRING REVERB

Expressive spring reverb for macOS (Apple Silicon + Intel). Classicus brings a stronger vintage spring voice, Novus has a more audible voltage-sag dip and recovery, and Somnium keeps a dark, enveloping halo. Intensity reaches longer decay and more body/presence on the wet return, while the direct path remains uncoloured.

## Status

Version 0.13.0 is an engineering build targeting JUCE 9.0.4. The local macOS build script compiles for the architecture of the Mac running it, executes the DSP regression test, and creates an unsigned test installer with a SHA-256 checksum.

The three modes are **Classicus / Spring** (rounded, zero-centred vintage drive), **Novus / Sag** (power-stage compression plus Voltage Sag: amplitude dip and recovery), and **Somnium / Halo** (soft peak rounding with a dark, wet-field halo). Drive uses a different smooth, bounded saturation curve per mode and remains wet-only. The mode-dependent Character control is explicitly labelled Spring, Sag, or Halo. The small **IN** and **OUT** faders provide smoothed input/output trim. The central **Fiuso** button adds a DC-blocked octave-up harmonic series and related even harmonics to the spring excitation; the on/off state is smoothed and the dry path stays free of the generated layer. Mode-specific colours are crossfaded continuously; the dry path is not pitch-modulated or time-shifted. Intensity controls decay and wet presence, while Tone shapes the wet return.

## Build and test locally on macOS

Requirements:
- macOS with Apple's Xcode Command Line Tools (or Xcode)
- CMake 3.22 or newer
- Internet access on the first build so CMake can fetch JUCE 9.0.4

Install Command Line Tools if needed:

    xcode-select --install

Install CMake if needed (with Homebrew):

    brew install cmake

From the repository root, run:

    chmod +x scripts/build_macos.sh
    ./scripts/build_macos.sh

The script builds AU and VST3 for the Mac's native architecture, runs `FUTURO_DSP_SMOKE_TEST`, checks the plug-in bundles, and creates:
- `dist/Spiritus-Spring-Reverb-0.13.0-macOS-arm64.pkg` on Apple Silicon
- `dist/Spiritus-Spring-Reverb-0.13.0-macOS-x86_64.pkg` on Intel
- the corresponding `.pkg.sha256` checksum file

A successful build and DSP smoke test are necessary but not sufficient for audio-quality validation. The installer is unsigned and not notarized. Do not distribute it as a signed/notarized release.

## GitHub Actions

The optional workflow at `.github/workflows/macos-dsp.yml` builds and tests the current macOS ARM64 configuration on GitHub-hosted runners. To avoid consuming GitHub-hosted runner minutes, build locally using the script above.

## DSP validation

The automated DSP suite covers finite output, stable decay, mode differentiation, Character and Tone response, mode-specific Drive response, the Fiuso harmonic layer, IN/OUT gain-trim response, output headroom, dry-path pitch lock, and dry-path isolation from Drive.

## Validation still needed

The plug-in must still be listened to and validated in the intended DAW, including automation, preset/state recall, tail stability, CPU usage and different sample rates/buffer sizes. The nonlinear stages are not oversampled, so extreme Drive or Fiuso settings can still generate some aliasing. Fiuso shapes the spring excitation with harmonic content; it is not a pitch-tracking polyphonic harmonizer. Objective tests cannot replace listening in the target DAW.

## Installation locations

- `/Library/Audio/Plug-Ins/VST3/Spiritus Spring Reverb.vst3`
- `/Library/Audio/Plug-Ins/Components/Spiritus Spring Reverb.component`

Quit the DAW before installing and restart it afterward. Use `scripts/uninstall_macos.sh` to remove the plug-in bundles.
