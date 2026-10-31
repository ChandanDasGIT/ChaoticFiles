#!/usr/bin/env bash
# Installs ChaoticFiles for the current user. No root needed.
# Run it from the unpacked folder:  ./install-app.sh
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [ ! -x "$HERE/app/AppRun" ]; then
  echo "The 'app' folder is missing. Run this script from the unpacked ChaoticFiles folder." >&2
  exit 1
fi

DATA="${XDG_DATA_HOME:-$HOME/.local/share}"
APP_DIR="$HOME/.local/lib/ChaoticFiles"   # not $DATA/ChaoticFiles: the app keeps its own data there
BIN_DIR="$HOME/.local/bin"
MENU_DIR="$DATA/applications"
ICON_DIR="$DATA/icons/hicolor/scalable/apps"

mkdir -p "$(dirname "$APP_DIR")" "$BIN_DIR" "$MENU_DIR" "$ICON_DIR"

# the program
rm -rf "$APP_DIR"
cp -a "$HERE/app" "$APP_DIR"

# the "ChaoticFiles" command
ln -sf "$APP_DIR/AppRun" "$BIN_DIR/ChaoticFiles"

# the icon
cp "$APP_DIR/usr/share/icons/hicolor/scalable/apps/ChaoticFiles.svg" "$ICON_DIR/ChaoticFiles.svg"

# the menu entry (the shipped one, pointing at the installed copy)
SRC="$APP_DIR/usr/share/applications/ChaoticFiles.desktop"
if [ -f "$SRC" ]; then
  sed -e "s|^Exec=.*|Exec=\"$APP_DIR/AppRun\" %F|" \
      -e "s|^Icon=.*|Icon=ChaoticFiles|" \
      "$SRC" > "$MENU_DIR/ChaoticFiles.desktop"
else
  cat > "$MENU_DIR/ChaoticFiles.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=ChaoticFiles
Exec="$APP_DIR/AppRun" %F
Icon=ChaoticFiles
Categories=System;FileTools;FileManager;
Terminal=false
EOF
fi

if command -v update-desktop-database > /dev/null 2>&1; then
  update-desktop-database "$MENU_DIR" > /dev/null 2>&1 || true
fi

echo "ChaoticFiles is installed."
echo "  Program : $APP_DIR"
echo "  Command : $BIN_DIR/ChaoticFiles"
echo "  Menu    : $MENU_DIR/ChaoticFiles.desktop"
case ":$PATH:" in
  *":$BIN_DIR:"*) ;;
  *) echo "Note: $BIN_DIR is not on your PATH, so the 'ChaoticFiles' command will not work until you add it." ;;
esac
