#!/usr/bin/env bash
# Removes what sudo ./install.sh installed. Your files and settings are not touched.
set -e

if [ "$EUID" -ne 0 ]; then
  echo "Please run as root (e.g., sudo ./uninstall.sh)"
  exit 1
fi

rm -f /usr/local/bin/appChaoticFiles /usr/local/bin/ChaoticFiles
rm -f /usr/local/share/applications/ChaoticFiles.desktop
rm -f /usr/local/share/icons/hicolor/scalable/apps/ChaoticFiles.svg

if command -v update-desktop-database &> /dev/null; then
  update-desktop-database /usr/local/share/applications 2>/dev/null || true
fi

echo "ChaoticFiles has been removed."
