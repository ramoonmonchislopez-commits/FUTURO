# GitHub build guide

The workflow at .github/workflows/macos-build.yml builds FUTURO on a GitHub-hosted Apple Silicon macOS runner and uploads:
- VST3 bundle archive
- Audio Unit bundle archive
- unsigned local test installer (.pkg)
- SHA-256 checksums

A push to main triggers the workflow. To see the build, open the repository Actions tab and select “FUTURO macOS Build”. Download the artifact named FUTURO-Spring-Reverb-macOS-arm64 after the run succeeds.

The build is pinned to JUCE 9.0.4 in CMake. The workflow compiles the native arm64 version intended for Apple Silicon.

Important: a green workflow only confirms compilation, packaging and file-presence checks. It does not prove audio quality or host compatibility. Test it in Ableton Live or the intended DAW. The package is unsigned and not notarized; public distribution requires appropriate Developer ID signing and Apple notarization.
