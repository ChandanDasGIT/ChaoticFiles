#!/usr/bin/env bash
# Removes what install-app.sh installed. Your files and settings are not touched.
set -euo pipefail

DATA="${XDG_DATA_HOME:-$HOME/.local/share}"

rm -rf  "$HOME/.local/lib/ChaoticFiles" "$HOME/.local/lib/ChaoticFiles.update"
rm -f   "$HOME/.local/lib/ChaoticFiles.update.failed"
rm -f   "$HOME/.local/bin/ChaoticFiles"
rm -f   "$DATA/applications/ChaoticFiles.desktop"
rm -f   "$DATA/icons/hicolor/scalable/apps/ChaoticFiles.svg"

if command -v update-desktop-database > /dev/null 2>&1; then
  update-desktop-database "$DATA/applications" > /dev/null 2>&1 || true
fi

echo "ChaoticFiles has been removed."
