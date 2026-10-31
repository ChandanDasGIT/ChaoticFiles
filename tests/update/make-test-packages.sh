#!/usr/bin/env bash
# Builds what run-update-tests.py installs: versions 1.0 and 1.1 with a test key, pointing at a local server, plus
# broken and forged copies of 1.1. Everything goes in the work folder. The real key is not used.
#
# Usage:  tests/update/make-test-packages.sh <work folder>
set -euo pipefail

WORK="$(realpath "${1:?usage: $0 <work folder>}")"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TOOLS="$ROOT/tools"
export APPIMAGE_EXTRACT_AND_RUN=1 ARCH=x86_64
PACKAGES="$WORK/packages"
rm -rf "$PACKAGES"; mkdir -p "$PACKAGES" "$WORK/keys"

key="$WORK/keys/test.pem"
other="$WORK/keys/other.pem"
openssl genpkey -algorithm ed25519 -out "$key"
openssl genpkey -algorithm ed25519 -out "$other"
public="$(openssl pkey -in "$key" -pubout -outform DER | tail -c 32 | od -An -tx1 | tr -d ' \n')"
if grep -q "$public" "$ROOT/packaging/update-public-key.txt"; then echo "test key is the real key?" >&2; exit 1; fi

export PACKAGING_CMAKE_ARGS="-DUPDATE_PUBLIC_KEY=$public -DUPDATE_API_URL=http://127.0.0.1:18765/api -DUPDATE_SITE_URL=http://127.0.0.1:18765/site -DCHAOTICFILES_TEST_DRIVER=ON"

sign() {   # sign <folder> <version> [key]: the same manifest as packaging/sign-update.sh
  local sum
  sum() { sha256sum -- "$1/$2" | cut -d' ' -f1; }
  {
    echo "ChaoticFiles update manifest 1"
    echo "version $2"
    echo "tarball ChaoticFiles-$2-linux-x86_64.tar.xz $(sum "$1" "ChaoticFiles-$2-linux-x86_64.tar.xz")"
    echo "appimage ChaoticFiles-$2-x86_64.AppImage $(sum "$1" "ChaoticFiles-$2-x86_64.AppImage")"
  } > "$1/update-manifest.txt"
  openssl pkeyutl -sign -inkey "${3:-$key}" -rawin -in "$1/update-manifest.txt" -out "$1/update-manifest.txt.sig"
}

for version in 1.0 1.1; do
  (cd "$ROOT" && ./packaging/build-appimage.sh "$version") > "$WORK/build-$version.log" 2>&1
  mkdir "$PACKAGES/$version"
  cp "$ROOT/dist/"*.AppImage "$ROOT/dist/"*.tar.xz "$PACKAGES/$version/"
  sign "$PACKAGES/$version" "$version"
  rm -rf "$WORK/AppDir-$version"; cp -a "$ROOT/AppDir" "$WORK/AppDir-$version"
done

# the plain install of an "old" version, for the copy that needs root
rm -rf "$WORK/build-old-plain"
cmake -S "$ROOT" -B "$WORK/build-old-plain" -G Ninja -DCMAKE_BUILD_TYPE=Release -DAPP_VERSION=1.0 $PACKAGING_CMAKE_ARGS > /dev/null 2>&1
cmake --build "$WORK/build-old-plain" --target appChaoticFiles > "$WORK/build-plain.log" 2>&1

broken() {   # broken <name> <what AppRun does instead>
  local folder="$PACKAGES/1.1-$1" app="$WORK/AppDir-broken-$1" package="ChaoticFiles-1.1-linux-x86_64"
  mkdir "$folder"
  rm -rf "$app"; cp -a "$WORK/AppDir-1.1" "$app"
  printf '#!/bin/sh\nHERE="$(dirname "$(readlink -f "$0")")"\n%s\n' "$2" > "$app/AppRun"
  chmod +x "$app/AppRun"
  "$TOOLS/appimagetool-x86_64.AppImage" --no-appstream "$app" "$folder/ChaoticFiles-1.1-x86_64.AppImage" > /dev/null 2>&1
  local stage="$WORK/stage-$1"; rm -rf "$stage"; mkdir -p "$stage/$package"
  cp -a "$app" "$stage/$package/app"
  cp "$ROOT/packaging/install-app.sh" "$ROOT/packaging/uninstall-app.sh" "$stage/$package/"
  XZ_OPT=-T0 tar -C "$stage" -cJf "$folder/$package.tar.xz" "$package"
  sign "$folder" 1.1
  rm -rf "$app" "$stage"
}
broken exits 'exit 3'
broken crashes 'kill -SEGV $$'
broken hangs 'sleep 100000'
# the real window opens, then dies before it has run for two seconds
broken dies-early 'export LD_LIBRARY_PATH="$HERE/usr/lib"; "$HERE/usr/bin/appChaoticFiles" "$@" & p=$!; sleep 1; kill -SEGV $p; wait $p'

forged() { mkdir "$PACKAGES/1.1-$1"; cp "$PACKAGES/1.1/"* "$PACKAGES/1.1-$1/"; }

forged wrong-key
openssl pkeyutl -sign -inkey "$other" -rawin -in "$PACKAGES/1.1-wrong-key/update-manifest.txt" -out "$PACKAGES/1.1-wrong-key/update-manifest.txt.sig"

forged tampered-file                                # signed correctly, then the file changes
printf 'x' >> "$PACKAGES/1.1-tampered-file/ChaoticFiles-1.1-linux-x86_64.tar.xz"
printf 'x' >> "$PACKAGES/1.1-tampered-file/ChaoticFiles-1.1-x86_64.AppImage"

forged missing-signature
rm "$PACKAGES/1.1-missing-signature/update-manifest.txt.sig"

forged edited-manifest                              # the file is forged and the manifest edited to match it
printf 'x' >> "$PACKAGES/1.1-edited-manifest/ChaoticFiles-1.1-linux-x86_64.tar.xz"
new="$(sha256sum "$PACKAGES/1.1-edited-manifest/ChaoticFiles-1.1-linux-x86_64.tar.xz" | cut -d' ' -f1)"
sed -i "s/^\(tarball [^ ]* \).*/\1$new/" "$PACKAGES/1.1-edited-manifest/update-manifest.txt"

forged other-version                                # really signed, but for a version that is not the one on offer
sed -i 's/^version 1.1$/version 9.9/' "$PACKAGES/1.1-other-version/update-manifest.txt"
sed -i 's/ChaoticFiles-1.1-/ChaoticFiles-9.9-/' "$PACKAGES/1.1-other-version/update-manifest.txt"
openssl pkeyutl -sign -inkey "$key" -rawin -in "$PACKAGES/1.1-other-version/update-manifest.txt" -out "$PACKAGES/1.1-other-version/update-manifest.txt.sig"

ls "$PACKAGES"
