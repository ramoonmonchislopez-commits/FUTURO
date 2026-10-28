#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT/build-macos"
DEPLOYMENT_TARGET="11.0"

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "ERROR: this script must run on macOS." >&2
  exit 2
fi
if ! command -v cmake >/dev/null 2>&1; then
  echo "ERROR: CMake 3.22+ is required. Install it with: brew install cmake" >&2
  exit 2
fi
if ! xcode-select -p >/dev/null 2>&1; then
  echo "ERROR: Apple Command Line Tools are missing. Run: xcode-select --install" >&2
  exit 2
fi

# Build natively to avoid requiring a second architecture's toolchain.
ARCH="$(uname -m)"
case "$ARCH" in
  arm64|x86_64) ;;
  *)
    echo "ERROR: unsupported Mac architecture: $ARCH" >&2
    exit 2
    ;;
esac

VERSION="$(sed -n 's/^project(SPIRITUS_SPRING VERSION \([0-9.]*\).*/\1/p' "$ROOT/CMakeLists.txt")"
if [[ -z "$VERSION" ]]; then
  echo "ERROR: could not read project version from CMakeLists.txt." >&2
  exit 2
fi

cmake --version
cmake -S "$ROOT" -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="$ARCH" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="$DEPLOYMENT_TARGET"

cmake --build "$BUILD_DIR" --config Release \
  --target FUTURO_SPRING_AU FUTURO_SPRING_VST3 FUTURO_DSP_SMOKE_TEST \
  --parallel "$(sysctl -n hw.ncpu)"

echo "Running FUTURO DSP regression tests..."
"$BUILD_DIR/FUTURO_DSP_SMOKE_TEST"

VST3_PATH="$(find "$BUILD_DIR" -type d -name 'Spiritus Spring Reverb.vst3' -print -quit)"
AU_PATH="$(find "$BUILD_DIR" -type d -name 'Spiritus Spring Reverb.component' -print -quit)"
if [[ -z "$VST3_PATH" || ! -d "$VST3_PATH" ]]; then
  echo "ERROR: build completed but VST3 bundle was not found." >&2
  exit 3
fi
if [[ -z "$AU_PATH" || ! -d "$AU_PATH" ]]; then
  echo "ERROR: build completed but AU bundle was not found." >&2
  exit 3
fi

# Re-sign after JUCE has generated all bundle resources.
codesign --force --deep --sign - "$VST3_PATH"
codesign --force --deep --sign - "$AU_PATH"
codesign --verify --deep --strict --verbose=2 "$VST3_PATH"
codesign --verify --deep --strict --verbose=2 "$AU_PATH"

VST3_BINARY="$VST3_PATH/Contents/MacOS/Spiritus Spring Reverb"
AU_BINARY="$AU_PATH/Contents/MacOS/Spiritus Spring Reverb"
for binary in "$VST3_BINARY" "$AU_BINARY"; do
  [[ -f "$binary" ]] || { echo "ERROR: missing binary: $binary" >&2; exit 3; }
  file "$binary"
  lipo "$binary" -verify_arch "$ARCH"
done

PKGROOT="$ROOT/dist/pkgroot"
rm -rf "$PKGROOT"
mkdir -p "$PKGROOT/Library/Audio/Plug-Ins/VST3"
mkdir -p "$PKGROOT/Library/Audio/Plug-Ins/Components"
cp -R "$VST3_PATH" "$PKGROOT/Library/Audio/Plug-Ins/VST3/"
cp -R "$AU_PATH" "$PKGROOT/Library/Audio/Plug-Ins/Components/"

mkdir -p "$ROOT/dist"
ARCH_LABEL="$ARCH"
PKG="$ROOT/dist/Spiritus-Spring-Reverb-$VERSION-macOS-$ARCH_LABEL.pkg"
rm -f "$PKG" "$PKG.sha256"
pkgbuild \
  --root "$PKGROOT" \
  --identifier "audio.spiritus.springreverb" \
  --version "$VERSION" \
  --install-location / \
  --ownership recommended \
  "$PKG"

pkgutil --payload-files "$PKG" | grep -E 'Audio/Plug-Ins/(VST3|Components)/Spiritus Spring Reverb' >/dev/null || {
  echo "ERROR: installer payload does not contain the expected plug-in bundles." >&2
  exit 4
}
shasum -a 256 "$PKG" > "$PKG.sha256"

echo "Build, DSP tests, and packaging completed:"
echo "  $PKG"
echo "  $PKG.sha256"
echo "WARNING: this local test installer is unsigned and not notarized. Test it in the target DAW."
