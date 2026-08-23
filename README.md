# Mnemis

<!-- HADS: EN Document -->

Mnemis — A high-performance, extensible multimedia library and media center for images, audio, and video.

---

## 📌 Context
> **TL;DR:** Mnemis is currently in Phase 14 of development, with a consolidated Playback Core.

## 🛠️ Features
- **Database:** SQLite structured and versioned with migrations.
- **File System:** Agnostic (currently StdFileSystem).
- **Metadata Indexer:** Multi-format (TagLib, FFmpeg, StbImage).
- **Cache:** Async optimized thumbnail cache.
- **Playback Core:** UI-agnostic, using `libmpv` as default backend.
- **UI:** Reactive interface with Qt/QML and C++ ViewModels.

## 🧰 Technologies
- C++20
- Qt 6 (Core, Gui, Qml, Quick)
- CMake
- SQLite3
- FFmpeg
- libmpv
- TagLib
- GoogleTest / GoogleMock

## 🚀 Build Instructions

```bash
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
```

## 🧪 Testing

```bash
cd build
ctest --output-on-failure
# Or run binary directly:
./bin/mnemis_tests
```

## 📄 License
This project is licensed under the **GNU GPL v3**.
