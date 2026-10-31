#!/usr/bin/env bash
# Writes dist/update-manifest.txt and signs it: dist/update-manifest.txt.sig. The app checks that signature with
# the public key built into it before it installs an update (see wrapper/updateWrap/updatecontroller.h).
#
# Usage:  ./packaging/sign-update.sh 1.1
# Needs:  UPDATE_SIGNING_KEY_BASE64   the key file, in base64
#         UPDATE_SIGNING_KEY_PASSWORD its password
# The release workflow runs this with its two secrets.
set -euo pipefail

VERSION="${1:?usage: $0 <version>   for example: $0 1.1}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PUBLIC_KEY="$(tr -d '[:space:]' < "$ROOT/packaging/update-public-key.txt")"
cd "$ROOT/dist"

TARBALL="ChaoticFiles-$VERSION-linux-x86_64.tar.xz"
APPIMAGE="ChaoticFiles-$VERSION-x86_64.AppImage"

sha() { sha256sum -- "$1" | cut -d' ' -f1; }
{
  echo "ChaoticFiles update manifest 1"
  echo "version $VERSION"
  echo "tarball $TARBALL $(sha "$TARBALL")"
  echo "appimage $APPIMAGE $(sha "$APPIMAGE")"
} > update-manifest.txt

key="$(mktemp)"
trap 'rm -f "$key"' EXIT
printf '%s' "$UPDATE_SIGNING_KEY_BASE64" | base64 -d > "$key"

# A release signed by any other key would be refused by every copy of the app, so that stops here.
found="$(openssl pkey -in "$key" -passin env:UPDATE_SIGNING_KEY_PASSWORD -pubout -outform DER | tail -c 32 | od -An -tx1 | tr -d ' \n')"
if [ "$found" != "$PUBLIC_KEY" ]; then
  echo "This signing key does not match packaging/update-public-key.txt." >&2
  exit 1
fi

openssl pkeyutl -sign -inkey "$key" -passin env:UPDATE_SIGNING_KEY_PASSWORD \
  -rawin -in update-manifest.txt -out update-manifest.txt.sig

echo "Signed the update for version $VERSION."
