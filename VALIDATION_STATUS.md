# FUTURO 0.14.0 validation status

Status: engineering/test build; not a verified commercial release.

Implemented in this branch:
- Retro-futurist dark glass panel, metallic/neon controls, and upright green-and-blue angel pair with hand-drawn feather wings, a halo, and mode-specific spark/lightning/moon accents.
- Smoothed Intensity-to-mood animation: low settings add frowns and tears, high settings ease into smiling expressions, horns, a devil tail, warmer orbital accents and wider wings.
- Editable factory preset menu with host parameter notifications and five more strongly contrasted voices.
- The uninitialised plug-in opens on Copper Bloom's Classicus reference values; the default APVTS values and factory preset agree.
- Perceptually shaped Character response, checked at intermediate settings in all three modes.
- Fiuso's rectifier state removes DC without discarding lower harmonic body; a Tone-controlled low-pass reduces brittle upper harmonics before the parallel saturation stages.
- Somnium uses more diffuse and decorrelated stereo delay patterns, a feedback ceiling of 0.91 (below unity), more tail level, lower early-reflection emphasis, and gentler outer saturation stages to avoid overdriving the wet return. The regression measures tail bloom after the initial attack and reports each failed condition separately.
- DSP regressions for finite output, bounded peaks, stable decay, Character travel, Fiuso audibility, and Somnium tail growth.
- The impulse test waits 150 ms for parameter smoothing and the mode crossfade to settle before injecting audio, so each test actually exercises its requested mode.

Not yet verified: successful CI on the final commit, AU/VST3 host validation, Ableton Live loading, listening-based quality, CPU use, real-time safety, and state/automation across full DAW sessions.

Known limitation: saturation is not oversampled and may alias at extreme settings. The generated test package is ad hoc signed and not notarized. Do not distribute commercially until audio and packaging checks pass.
