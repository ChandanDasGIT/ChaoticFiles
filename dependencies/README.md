Third-party code, one folder per library.

- blake3/: the official BLAKE3 C code, version 1.5.5, portable part only. Public domain (CC0) or Apache 2.0.
  From https://github.com/BLAKE3-team/BLAKE3/tree/1.5.5/c, unchanged.

The app also uses these at run time when they are installed, and says so when one is missing:
libmagic (detailed file types), ffprobe and ffmpeg (audio, video), pdfinfo (PDF), tar, zip, unzip, 7z (archives).
