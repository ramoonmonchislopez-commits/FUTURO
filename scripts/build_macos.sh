#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT/build-macos"
ARCHS="arm64;x86_64"
ARCH_LABEL="universal"
DEPLOYMENT_TARGET="11.0"
VERSION="0.6.0"

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

cmake --version
cmake -S "$ROOT" -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="$ARCHS" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="$DEPLOYMENT_TARGET"
cmake --build "$BUILD_DIR" --config Release --parallel "$(sysctl -n hw.ncpu)"

VST3_PATH="$(find "$BUILD_DIR" -type d -name 'FUTURO Spring Reverb.vst3' -print -quit)"
AU_PATH="$(find "$BUILD_DIR" -type d -name 'FUTURO Spring Reverb.component' -print -quit)"
if [[ -z "$VST3_PATH" || ! -d "$VST3_PATH" ]]; then
  echo "ERROR: build completed but VST3 bundle was not found." >&2
  exit 3
fi
if [[ -z "$AU_PATH" || ! -d "$AU_PATH" ]]; then
  echo "ERROR: build completed but AU bundle was not found." >&2
  exit 3
fi

# JUCE's VST3 manifest helper may write moduleinfo.json after the initial link/sign pass.
# Re-sign after all bundle resources have been generated so the resource seal is current.
codesign --force --deep --sign - "$VST3_PATH"
codesign --force --deep --sign - "$AU_PATH"
codesign --verify --deep --strict --verbose=2 "$VST3_PATH"
codesign --verify --deep --strict --verbose=2 "$AU_PATH"

# Assert that both architectures are actually present before packaging.
VST3_BINARY="$VST3_PATH/Contents/MacOS/FUTURO Spring Reverb"
AU_BINARY="$AU_PATH/Contents/MacOS/FUTURO Spring Reverb"
for binary in "$VST3_BINARY" "$AU_BINARY"; do
  [[ -f "$binary" ]] || { echo "ERROR: missing binary: $binary" >&2; exit 3; }
  file "$binary"
  lipo "$binary" -verify_arch arm64 x86_64
done

PKGROOT="$ROOT/dist/pkgroot"
rm -rf "$PKGROOT"
mkdir -p "$PKGROOT/Library/Audio/Plug-Ins/VST3"
mkdir -p "$PKGROOT/Library/Audio/Plug-Ins/Components"
cp -R "$VST3_PATH" "$PKGROOT/Library/Audio/Plug-Ins/VST3/"
cp -R "$AU_PATH" "$PKGROOT/Library/Audio/Plug-Ins/Components/"

mkdir -p "$ROOT/dist"
PKG="$ROOT/dist/FUTURO-Spring-Reverb-$VERSION-$ARCH_LABEL.pkg"
rm -f "$PKG"
pkgbuild \
  --root "$PKGROOT" \
  --identifier "audio.futuro.springreverb.pkg" \
  --version "$VERSION" \
  --install-location / \
  --ownership recommended \
  "$PKG"

pkgutil --check-signature "$PKG" || true
pkgutil --payload-files "$PKG" | grep -E 'Audio/Plug-Ins/(VST3|Components)/FUTURO Spring Reverb' >/dev/null || {
  echo "ERROR: installer payload does not contain the expected plug-in bundles." >&2
  exit 4
}

echo "Build and package completed: $PKG"
echo "WARNING: local test installer is unsigned and not notarized. A successful build is not proof of sound quality; test in the target DAW."
