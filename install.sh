#!/usr/bin/env bash
# Builds and installs ChaoticFiles for every user of this computer. Run it from the project folder:  sudo ./install.sh
# You only need this to install from source. The release tarball and the AppImage need no root.
set -e

if [ "$EUID" -ne 0 ]; then
  echo "Please run as root (e.g., sudo ./install.sh)"
  exit 1
fi

BUILD_DIR="build-install"
if [ ! -d "$BUILD_DIR" ]; then
  cmake -B "$BUILD_DIR" -S . -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local
fi
cmake --build "$BUILD_DIR" --parallel
cmake --install "$BUILD_DIR"

if command -v update-desktop-database &> /dev/null; then
  update-desktop-database /usr/local/share/applications 2>/dev/null || true
fi
if command -v gtk-update-icon-cache &> /dev/null; then
  gtk-update-icon-cache -f -t /usr/local/share/icons/hicolor 2>/dev/null || true
fi

# A copy from the release tarball (install-app.sh) lives in the user's home and would open instead of this one.
# Only that copy goes: ~/.local/share/ChaoticFiles and the settings stay. Under sudo, SUDO_USER tells whose home it is.
if [ -n "${SUDO_USER:-}" ] && [ "$SUDO_USER" != "root" ]; then
  USER_HOME="$(getent passwd "$SUDO_USER" | cut -d: -f6)"
  if [ -n "$USER_HOME" ] && [ -x "$USER_HOME/.local/lib/ChaoticFiles/AppRun" ]; then
    rm -rf "$USER_HOME/.local/lib/ChaoticFiles" "$USER_HOME/.local/lib/ChaoticFiles.update"
    rm -f "$USER_HOME/.local/lib/ChaoticFiles.update.failed"
    if [ -L "$USER_HOME/.local/bin/ChaoticFiles" ]; then rm -f "$USER_HOME/.local/bin/ChaoticFiles"; fi
    rm -f "$USER_HOME/.local/share/applications/ChaoticFiles.desktop"
    rm -f "$USER_HOME/.local/share/icons/hicolor/scalable/apps/ChaoticFiles.svg"
    if command -v update-desktop-database &> /dev/null; then
      sudo -u "$SUDO_USER" update-desktop-database "$USER_HOME/.local/share/applications" 2>/dev/null || true
    fi
    echo "Removed the copy installed from the release tarball, so this version is the one that opens."
  fi
fi

echo "ChaoticFiles installed successfully!"
