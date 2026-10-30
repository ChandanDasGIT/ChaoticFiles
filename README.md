# ChaoticFiles

A file manager for Linux, written in C++20 and Qt 6 QML. The engine is plain C++ and does not depend on Qt.

Status: early. It browses files; it does not copy, move or delete them yet. See `ROADMAP.md`.

## Build

Needs CMake, Ninja, a C++20 compiler and Qt 6.10 or newer (Quick and DBus).

    cmake -S . -B build -G Ninja
    cmake --build build
    build/appChaoticFiles

Folders named on the command line open in tabs: `build/appChaoticFiles ~/Music ~/Downloads`

## Test

    ctest --test-dir build --output-on-failure

## Layout

- `backend/`: the engine. Plain C++20, built as a library that does not link Qt.
- `wrapper/`: Qt C++ that connects the engine to QML.
- `frontend/`: the QML interface. All colours are in `Theme.qml` and all icons in `Icons.qml`.
- `dependencies/`: third-party code, when something needs it.
- `tests/`: engine tests.

`EducationPurpose/architecture.txt` lists every file.

## License

GNU GPL v3. See `LICENSE`.
