# FUTURO 0.14.0 validation status

Status: engineering/test build; not a verified commercial release.

Implemented in this branch:
- Editable factory preset menu with host parameter notifications.
- Perceptually shaped Character response, checked at intermediate settings in all three modes.
- Fiuso's rectifier state removes DC without discarding lower harmonic body; a Tone-controlled low-pass reduces brittle upper harmonics before the parallel saturation stages.
- Somnium uses more diffuse and decorrelated stereo delay patterns, a slightly longer feedback range, more tail level, and lower early-reflection emphasis.
- DSP regressions for finite output, bounded peaks, stable decay, Character travel, Fiuso audibility, and Somnium tail growth.

Not yet verified: successful CI on the final commit, AU/VST3 host validation, Ableton Live loading, listening-based quality, CPU use, real-time safety, and state/automation across full DAW sessions.

Known limitation: saturation is not oversampled and may alias at extreme settings. The generated test package is ad hoc signed and not notarized. Do not distribute commercially until audio and packaging checks pass.
