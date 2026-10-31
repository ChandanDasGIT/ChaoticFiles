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

## Build

Needs CMake, Ninja, a C and C++20 compiler, and Qt 6.10 or newer (Quick and DBus).

    cmake -S . -B build -G Ninja
    cmake --build build
    build/appChaoticFiles

Folders named on the command line open in tabs: `build/appChaoticFiles ~/Music ~/Downloads`

## Optional programs

The app works without these, and says what is missing when one is needed:
libmagic (detailed file types), ffprobe (audio and video details), ffmpegthumbnailer or ffmpeg (video thumbnails),
pdfinfo and pdftoppm (PDF), tar, zip, unzip and 7z (archives), udisksctl and lsblk (drives), gio with gvfs (NAS).

## Test

    ctest --test-dir build --output-on-failure

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
