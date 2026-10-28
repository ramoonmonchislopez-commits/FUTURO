# SPIRITUS — SPRING REVERB

Expressive spring reverb for macOS (Apple Silicon + Intel). Classicus brings a stronger vintage spring voice, Novus has a more audible voltage-sag dip and recovery, and Somnium keeps a dark, enveloping halo. Intensity reaches longer decay and more body/presence on the wet return, while the direct path remains uncoloured.

## Status

Version 0.14.0 is an engineering build targeting JUCE 9.0.4. It adds editable factory presets and DSP refinements to Character, Fiuso, and Somnium, plus a retro-futurist dark interface based on the hand-drawn angel pair. The pair's facial expression eases from sad at low Intensity to mischievous little devils at high Intensity, using a smoothed visual value independent of the DSP parameter. The local macOS build script compiles for the architecture of the Mac running it, executes the DSP regression suite, and creates an unsigned test installer with a SHA-256 checksum.

The three modes are **Classicus / Spring** (rounded, zero-centred vintage drive), **Novus / Sag** (power-stage compression plus Voltage Sag: amplitude dip and recovery), and **Somnium / Halo** (soft peak rounding with a dark, wet-field halo). Drive uses a different smooth, bounded saturation curve per mode and remains wet-only. The mode-dependent Character control is explicitly labelled Spring, Sag, or Halo. The small **IN** and **OUT** faders provide smoothed input/output trim. The central **Fiuso** button adds a DC-blocked octave-up harmonic series and related even harmonics to the spring excitation; the on/off state is smoothed and the dry path stays free of the generated layer. Mode-specific colours are crossfaded continuously; the dry path is not pitch-modulated or time-shifted. The interface's upright green-and-blue angel pair is drawn from the supplied sketch, animates its scale/breathing, wing spread, expression, tear fade, horns and tail in response to Intensity, and gains a different small accessory for Classicus, Novus and Somnium. Facial mood is smoothly interpolated over time to avoid snapping as the knob moves. Intensity controls decay and wet presence, while Tone shapes the wet return.

A **FACTORY PRESETS** menu provides five deliberately contrasted, editable voices: **Copper Bloom** (the default Classicus reference), **Studio Coil** (short and clean), **Voltage Kiss** (Novus sag and bite), **Lunar Halo** (Somnium dark and spacious with low Drive), and **Temple Bloom** (Somnium with Fiūso harmonic shimmer). Choosing a preset applies values through the host parameter API; every control remains editable and the DAW can save the result with the project.

Version 0.14.0 also makes Character more responsive through its full travel, gives Fiuso a true low-frequency DC blocker plus Tone-controlled harmonic softening, and makes Somnium denser and more spacious through diffusion and stereo decorrelation. Its feedback-loop character is retained, while the outer saturation stages are gentler to keep dense material smooth.

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
- `dist/Spiritus-Spring-Reverb-0.14.0-macOS-arm64.pkg` on Apple Silicon
- `dist/Spiritus-Spring-Reverb-0.14.0-macOS-x86_64.pkg` on Intel
- the corresponding `.pkg.sha256` checksum file

A successful build and DSP smoke test are necessary but not sufficient for audio-quality validation. The installer is unsigned and not notarized. Do not distribute it as a signed/notarized release.

## GitHub Actions

The optional workflow at `.github/workflows/macos-dsp.yml` builds and tests the current macOS ARM64 configuration on GitHub-hosted runners. To avoid consuming GitHub-hosted runner minutes, build locally using the script above.

## DSP validation

The automated DSP suite covers finite output, stable decay, mode differentiation, Character response at intermediate settings, Tone response, mode-specific Drive response, the Fiuso harmonic layer, Somnium tail expansion, IN/OUT gain-trim response, output headroom, dry-path pitch lock, and dry-path isolation from Drive.

## Validation still needed

The plug-in must still be listened to and validated in the intended DAW, including automation, preset/state recall, tail stability, CPU usage and different sample rates/buffer sizes. The nonlinear stages are not oversampled, so extreme Drive or Fiuso settings can still generate some aliasing. Fiuso shapes the spring excitation with harmonic content; it is not a pitch-tracking polyphonic harmonizer. Objective tests cannot replace listening in the target DAW.

## Installation locations

- `/Library/Audio/Plug-Ins/VST3/Spiritus Spring Reverb.vst3`
- `/Library/Audio/Plug-Ins/Components/Spiritus Spring Reverb.component`

Quit the DAW before installing and restart it afterward. Use `scripts/uninstall_macos.sh` to remove the plug-in bundles.
