# Mnemis

<!-- HADS: ES Document -->

Mnemis — Una biblioteca multimedia y centro multimedia de alto rendimiento y extensible para imágenes, audio y video.

---

## 📌 Contexto
> **Resumen:** Mnemis se encuentra en la Fase 14 de desarrollo, con un Playback Core consolidado.

## 🛠️ Características
- **Base de datos:** SQLite estructurada y versionada con migraciones.
- **Sistema de archivos:** Agnóstico (actualmente StdFileSystem).
- **Indexador de metadatos:** Multi-formato (TagLib, FFmpeg, StbImage).
- **Caché:** Caché de miniaturas asíncrona optimizada.
- **Playback Core:** Agnóstico a la UI, usa `libmpv` como backend por defecto.
- **UI:** Interfaz reactiva con Qt/QML y ViewModels en C++.

## 🧰 Tecnologías
- C++20
- Qt 6 (Core, Gui, Qml, Quick)
- CMake
- SQLite3
- FFmpeg
- libmpv
- TagLib
- GoogleTest / GoogleMock

## 🚀 Cómo Compilar

```bash
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
```

## 🧪 Cómo Ejecutar Tests

```bash
cd build
ctest --output-on-failure
# O ejecutar el binario directamente:
./bin/mnemis_tests
```

## 📄 Licencia
Este proyecto está licenciado bajo **GNU GPL v3**.
