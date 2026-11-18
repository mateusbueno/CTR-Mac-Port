#!/bin/bash
# CTR Native Build Script (macOS Apple Silicon / Darwin arm64)

set -e

if ! command -v clang &>/dev/null; then
    echo "ERROR: clang not found. Please install Xcode Command Line Tools: xcode-select --install"
    exit 1
fi

if ! command -v cmake &>/dev/null; then
    echo "ERROR: cmake not found. Please install via Homebrew: brew install cmake"
    exit 1
fi

echo "==> Configuring CTR-Native for macOS (arm64)..."
cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="arm64" \
  "-DCMAKE_POLICY_VERSION_MINIMUM=3.5"

echo "==> Building CTR-Native..."
cmake --build build -j$(sysctl -n hw.ncpu)

echo "==> Packaging macOS App Bundle (CTR Native.app)..."
APP_BUNDLE="build/CTR Native.app"
CONTENTS="$APP_BUNDLE/Contents"
MACOS_DIR="$CONTENTS/MacOS"
RESOURCES_DIR="$CONTENTS/Resources"

mkdir -p "$MACOS_DIR"
mkdir -p "$RESOURCES_DIR"

cp build/ctr_native "$MACOS_DIR/ctr_native"
chmod +x "$MACOS_DIR/ctr_native"

# Generate Info.plist
cat << 'EOF' > "$CONTENTS/Info.plist"
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>ctr_native</string>
    <key>CFBundleIdentifier</key>
    <string>com.ctr-native.mac</string>
    <key>CFBundleName</key>
    <string>CTR Native</string>
    <key>CFBundleDisplayName</key>
    <string>Crash Team Racing Native</string>
    <key>CFBundleIconFile</key>
    <string>AppIcon</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>0.1.0</string>
    <key>CFBundleVersion</key>
    <string>1</string>
    <key>LSMinimumSystemVersion</key>
    <string>11.0</string>
    <key>NSHighResolutionCapable</key>
    <true/>
    <key>NSSupportsAutomaticGraphicsSwitching</key>
    <true/>
</dict>
</plist>
EOF

# Generate PkgInfo
echo "APPL????" > "$CONTENTS/PkgInfo"

# Copy App Icon if available
if [ -f "externals/SDL/src/hidapi/testgui/TestGUI.app.in/Contents/Resources/Signal11.icns" ]; then
    cp "externals/SDL/src/hidapi/testgui/TestGUI.app.in/Contents/Resources/Signal11.icns" "$RESOURCES_DIR/AppIcon.icns"
fi

# Link or copy assets folder if present in root
if [ -d "assets" ]; then
    rm -rf "$RESOURCES_DIR/assets"
    ln -sf ../../../assets "$RESOURCES_DIR/assets"
    rm -rf "build/assets"
    ln -sf ../assets "build/assets"
fi

echo ""
echo "Build succeeded!"
echo "  CLI binary:  build/ctr_native"
echo "  App bundle:  build/CTR Native.app"
