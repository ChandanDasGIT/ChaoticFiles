#!/usr/bin/env bash
# Builds ChaoticFiles and puts these files into dist/:
#   ChaoticFiles-<version>-x86_64.AppImage
#   ChaoticFiles-<version>-linux-x86_64.tar.xz   (with install-app.sh and uninstall-app.sh inside)
#   SHA256SUMS.txt
#
# Usage:  ./packaging/build-appimage.sh 1.0
# Needs:  Qt 6.10 or newer (qmake6 or qmake on PATH, or QMAKE=/path/to/qmake6), cmake, curl, rsvg-convert, a C++20 compiler.
set -euo pipefail

VERSION="${1:?usage: $0 <version>   for example: $0 1.0}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

NAME=ChaoticFiles
ARCH=x86_64
APPDIR="$ROOT/AppDir"
DIST="$ROOT/dist"
TOOLS="$ROOT/tools"
BUILD="$ROOT/build-release"

export ARCH
export APPIMAGE_EXTRACT_AND_RUN=1   # run the downloaded AppImage tools without FUSE
export NO_STRIP=1                   # linuxdeploy's old "strip" breaks libraries from new distros
export PATH="$TOOLS:$PATH"

# ------------------------------------------------------------------ find Qt
QMAKE="${QMAKE:-$(command -v qmake6 || command -v qmake || true)}"
if [ -z "$QMAKE" ]; then
  echo "qmake not found. Put Qt's bin folder on PATH or run: QMAKE=/path/to/qmake6 $0 $VERSION" >&2
  exit 1
fi
export QMAKE
QT_PREFIX="$("$QMAKE" -query QT_INSTALL_PREFIX)"
QT_LIBS="$("$QMAKE" -query QT_INSTALL_LIBS)"
QT_PLUGINS="$("$QMAKE" -query QT_INSTALL_PLUGINS)"
QT_QML="$("$QMAKE" -query QT_INSTALL_QML)"
echo "Using Qt from $QT_PREFIX"

# ------------------------------------------------------------------ tools (downloaded once into tools/)
mkdir -p "$TOOLS"
fetch() {   # fetch <url> <file name>
  if [ ! -x "$TOOLS/$2" ]; then
    echo "Downloading $2"
    curl -fL --retry 3 -o "$TOOLS/$2" "$1"
    chmod +x "$TOOLS/$2"
  fi
}
fetch https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage \
      linuxdeploy-x86_64.AppImage
fetch https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage \
      linuxdeploy-plugin-qt-x86_64.AppImage
fetch https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage \
      appimagetool-x86_64.AppImage

# ------------------------------------------------------------------ build and install into AppDir
rm -rf "$APPDIR" "$DIST" "$BUILD"
mkdir -p "$DIST"

cmake -S . -B "$BUILD" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr \
  -DCMAKE_PREFIX_PATH="$QT_PREFIX" \
  -DAPP_VERSION="$VERSION" \
  ${PACKAGING_CMAKE_ARGS:-}   # only tests/update sets this: its own key and local server
cmake --build "$BUILD" -j"$(nproc)"
DESTDIR="$APPDIR" cmake --install "$BUILD"

DESKTOP="$APPDIR/usr/share/applications/$NAME.desktop"
ICON="$APPDIR/usr/share/icons/hicolor/scalable/apps/$NAME.svg"
for f in "$DESKTOP" "$ICON" "$APPDIR/usr/bin/appChaoticFiles"; do
  [ -e "$f" ] || { echo "Missing after install: $f" >&2; exit 1; }
done

# ------------------------------------------------------------------ let linuxdeploy collect Qt
# Only the QML files are scanned for imports (not the build folders).
QML_DIR="$BUILD/qml-sources"
mkdir -p "$QML_DIR"
cp Main.qml "$QML_DIR/"
cp -r frontend "$QML_DIR/"
export QML_SOURCES_PATHS="$QML_DIR"

# Wayland support. Qt 6.10 and newer have one plugin (libqwayland.so); older Qt has two.
PLATFORMS=""
for p in libqwayland.so libqwayland-generic.so libqwayland-egl.so; do
  if [ -f "$QT_PLUGINS/platforms/$p" ]; then PLATFORMS="${PLATFORMS:+$PLATFORMS;}$p"; fi
done
export EXTRA_PLATFORM_PLUGINS="$PLATFORMS"

EXTRA=""
for d in wayland-decoration-client wayland-graphics-integration-client wayland-shell-integration; do
  if [ -d "$QT_PLUGINS/$d" ]; then EXTRA="${EXTRA:+$EXTRA;}$d"; fi
done
export EXTRA_QT_PLUGINS="$EXTRA"

linuxdeploy-x86_64.AppImage \
  --appdir "$APPDIR" \
  --executable "$APPDIR/usr/bin/appChaoticFiles" \
  --desktop-file "$DESKTOP" \
  --icon-file "$ICON" \
  --plugin qt

# ------------------------------------------------------------------ plugins linuxdeploy leaves out
copy_plugin() {   # copy_plugin <folder> <file>: a Qt plugin, if this Qt has it
  if [ -f "$QT_PLUGINS/$1/$2" ]; then
    mkdir -p "$APPDIR/usr/plugins/$1"
    cp -L "$QT_PLUGINS/$1/$2" "$APPDIR/usr/plugins/$1/"
  fi
}

# Wayland OpenGL: without this plugin the app cannot draw under Wayland.
GFX="wayland-graphics-integration-client"
if [ -d "$QT_PLUGINS/$GFX" ]; then
  for f in "$QT_PLUGINS/$GFX"/*.so; do
    if [ ! -e "$f" ]; then continue; fi
    case "$f" in
      *-server.so) ;;   # server side plugins are not needed by an application
      *) copy_plugin "$GFX" "$(basename "$f")" ;;
    esac
  done
fi

# App icons are mostly SVG. The app does not link Qt Svg, so nothing asks for these.
copy_plugin iconengines libqsvgicon.so
copy_plugin imageformats libqsvg.so

for lib in libQt6WaylandClient.so.6 libQt6WaylandEglClientHwIntegration.so.6 libQt6WlShellIntegration.so.6 libQt6Svg.so.6; do
  if [ ! -e "$APPDIR/usr/lib/$lib" ] && [ -e "$QT_LIBS/$lib" ]; then
    cp -L "$QT_LIBS/$lib" "$APPDIR/usr/lib/"
  fi
done

# ------------------------------------------------------------------ put back untouched libraries and plugins
# The AppImage tools rewrite the headers of every library and plugin they bundle (to change where
# they look for other libraries). On some systems that damages them and the app crashes at start-up
# (it did for ChaoticNotepad on Fedora 44, in the Wayland libraries and the Wayland plugin).
# So every library, Qt plugin and QML module is put back exactly as it is in Qt or on this system.
# AppRun (written below) then sets the library path itself, so the rewritten headers are not needed.
original_lib() {   # original_lib <file name>  ->  path of the untouched 64-bit library, or nothing
  local src="$QT_LIBS/$1"
  if [ ! -e "$src" ]; then
    src="$( { ldconfig -p 2>/dev/null || /sbin/ldconfig -p 2>/dev/null || true; } \
            | awk -v l="$1" '$1 == l && /x86-64/ && !found { print $NF; found = 1 }' )"
  fi
  printf '%s' "$src"
}

# libraries
while IFS= read -r -d '' f; do
  src="$(original_lib "$(basename "$f")")"
  if [ -n "$src" ] && [ -e "$src" ]; then cp -L --remove-destination "$src" "$f"; fi
done < <(find "$APPDIR/usr/lib" -maxdepth 1 \( -type f -o -type l \) -name '*.so*' -print0)

# Qt plugins and QML modules (same relative path in Qt's own folders)
restore_tree() {   # restore_tree <folder in AppDir> <same folder in Qt>
  local f rel
  if [ ! -d "$1" ]; then return 0; fi
  while IFS= read -r -d '' f; do
    rel="${f#"$1"/}"
    if [ -f "$2/$rel" ]; then cp -L --remove-destination "$2/$rel" "$f"; fi
  done < <(find "$1" -type f -name '*.so*' -print0)
}
restore_tree "$APPDIR/usr/plugins" "$QT_PLUGINS"
restore_tree "$APPDIR/usr/qml" "$QT_QML"

# Qt finds its plugins and QML modules through this file, not through environment variables, because
# every app this one opens inherits its environment.
cat > "$APPDIR/usr/bin/qt.conf" <<'EOF'
[Paths]
Prefix = ../
Plugins = plugins
Imports = qml
Qml2Imports = qml
EOF

# AppRun: a small launcher that points the program at the bundled libraries.
# The program puts the user's LD_LIBRARY_PATH back as soon as it starts (see main.cpp).
rm -f "$APPDIR/AppRun"
cat > "$APPDIR/AppRun" <<'EOF'
#!/bin/sh
HERE="$(dirname "$(readlink -f "$0")")"
export CHAOTICFILES_USER_LD_LIBRARY_PATH="${LD_LIBRARY_PATH:-}"
export LD_LIBRARY_PATH="$HERE/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
exec "$HERE/usr/bin/appChaoticFiles" "$@"
EOF
chmod +x "$APPDIR/AppRun"

# ------------------------------------------------------------------ the AppImage
# appimagetool refuses an SVG here: .DirIcon has to be a PNG.
rm -f "$APPDIR/.DirIcon"
rsvg-convert -w 256 -h 256 "$ICON" -o "$APPDIR/.DirIcon"
appimagetool-x86_64.AppImage --no-appstream "$APPDIR" "$DIST/$NAME-$VERSION-$ARCH.AppImage"

# ------------------------------------------------------------------ the tarball
PKG="$NAME-$VERSION-linux-$ARCH"
STAGE="$BUILD/$PKG"
rm -rf "$STAGE"
mkdir -p "$STAGE"
cp -a "$APPDIR" "$STAGE/app"
cp packaging/install-app.sh packaging/uninstall-app.sh "$STAGE/"
chmod +x "$STAGE/install-app.sh" "$STAGE/uninstall-app.sh"
XZ_OPT=-T0 tar -C "$BUILD" -cJf "$DIST/$PKG.tar.xz" "$PKG"

# ------------------------------------------------------------------ checksums
(cd "$DIST" && sha256sum -- ./*.AppImage ./*.tar.xz > SHA256SUMS.txt)

echo
echo "Done. Files in $DIST:"
ls -lh "$DIST"
