# ChaoticFiles

A file manager for Linux, written in C++20 and Qt 6 QML. The engine is plain C++ and does not depend on Qt.

## Install

Download from [Releases](https://github.com/ChandanDasGIT/ChaoticFiles/releases). Linux x86_64 only.

**AppImage:** make it executable and run it.

    chmod +x ChaoticFiles-*-x86_64.AppImage
    ./ChaoticFiles-*-x86_64.AppImage

**Tarball:** installs for your user without root, and adds a menu entry and the `ChaoticFiles` command.

    tar -xf ChaoticFiles-*-linux-x86_64.tar.xz
    cd ChaoticFiles-*-linux-x86_64
    ./install-app.sh

`./uninstall-app.sh` removes it again and keeps your settings.

**From source, for every user:** `sudo ./install.sh` from the project folder, and `sudo ./uninstall.sh` to remove it. You only need root for this one. The tarball and the AppImage don't.

## Updates

Once a day the app asks GitHub for the latest version. Nothing else is sent. When a new one is out, a green dot appears in the status bar, beside the disk space. Click it to open **Settings > About**. To stop the daily check, turn off **Check for updates once a day** there. It is on by default.

Nothing updates until you choose to:

- **Tarball and AppImage:** **Update now** downloads the new version and checks its signature. It waits for running copies and moves if you want it to, restarts the app and opens your tabs again. Other ChaoticFiles windows close too, each after its own file operations, and open again with their tabs. If the new version does not start, the old one comes back and the app says so. Close any other ChaoticFiles windows first.
  An AppImage is replaced by the new file under its own name, such as `ChaoticFiles-1.1-x86_64.AppImage`, so there is nothing to rename. If you renamed yours, it keeps your name.
- **Installed with `sudo ./install.sh`, or anywhere you can't write:** this needs root, so the app offers the release page instead.

### How updates are signed

Every release has `update-manifest.txt`, which lists the version and the SHA-256 of each download, and `update-manifest.txt.sig`, its Ed25519 signature. The app has the public key built in and installs an update only if the signature, the version and the file's hash are all correct.

Public key:

```text
4c55692ce1e8e9495b3daabc71d175489dc118649942fc23864a41b1215cffaa
```

The private key is never in this repository. The release workflow reads it from the GitHub secrets `UPDATE_SIGNING_KEY_BASE64` and `UPDATE_SIGNING_KEY_PASSWORD`, and `packaging/sign-update.sh` signs the release. A tag without those secrets stops before anything is published.

## Build

Needs CMake, Ninja, a C and C++20 compiler, and Qt 6.10 or newer (Quick and DBus).

    cmake -S . -B build -G Ninja
    cmake --build build
    build/appChaoticFiles

Folders named on the command line open in tabs: `build/appChaoticFiles ~/Music ~/Downloads`

## Other programs

The app runs without these, and says what is missing when one is needed. Install the ones for what you use.
`ffmpeg` is worth having on any system: it is behind several features, and most distributions ship `ffprobe` with it.

| Program | What it is for |
| --- | --- |
| `ffmpeg` | Video thumbnails, and album art in formats other than FLAC and MP3 |
| `ffprobe` | Audio and video details |
| `ffmpegthumbnailer` | Quicker video thumbnails, used before `ffmpeg` |
| `pdfinfo`, `pdftoppm` (poppler) | PDF details and thumbnails |
| `tar`, `zip`, `unzip`, `7z` | Making and opening archives |
| `udisksctl`, `lsblk` | Listing and mounting drives |
| `gio` with gvfs | Network folders (NAS) |
| libmagic | Detailed file types |
| shared-mime-info | Exact file types by name |

## Test

    ctest --test-dir build --output-on-failure

The update system has its own tests, which use a test key, a local server and a throwaway home folder. They need a
Wayland session, and open real windows for a moment:

    tests/update/make-test-packages.sh /tmp/update-tests
    python3 tests/update/run-update-tests.py /tmp/update-tests

## Release

`./packaging/build-appimage.sh 1.0` puts the AppImage, tarball and checksums in `dist/`. An AppImage only runs on
systems as new as the one that built it, so public releases come from GitHub Actions on Ubuntu 22.04: pushing a tag
like `v1.0` builds and publishes one.

## Layout

- `backend/`: the engine. Plain C++20, built as a library that does not link Qt.
- `wrapper/`: Qt C++ that connects the engine to QML.
- `frontend/`: the QML interface. All colours are in `Theme.qml` and all icons in `Icons.qml`.
- `resources/`: files built into the app.
- `dependencies/`: third-party code, when something needs it.
- `tests/`: engine tests.

`EducationPurpose/architecture.txt` lists every file.

## License

GNU GPL v3. See `LICENSE`.
