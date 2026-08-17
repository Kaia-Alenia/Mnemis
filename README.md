# Mnemis

Mnemis — A high-performance, extensible multimedia library and media center for images, audio, and video.

## Estado Actual

El proyecto se encuentra en la Fase 14 del desarrollo, con el núcleo de reproducción (Playback Core) consolidado y migrado a la nueva arquitectura.

## Características Actuales
- Base de datos en SQLite estructurada y versionada con migraciones.
- Sistema de archivos agnóstico (actualmente StdFileSystem).
- Indexador de metadatos multi-formato (TagLib, FFmpeg, StbImage).
- Caché de miniaturas asíncrona optimizada.
- Playback Core agnóstico a la UI, usando `libmpv` como backend por defecto.
- Interfaz reactiva con Qt/QML y ViewModels en C++.

## Tecnologías
- C++20
- Qt 6 (Core, Gui, Qml, Quick)
- CMake
- SQLite3
- FFmpeg
- libmpv
- TagLib
- GoogleTest / GoogleMock

## Cómo Compilar

```bash
mkdir build && cd build
cmake ..
cmake --build . -j$(nproc)
```

## Cómo Ejecutar Tests

```bash
cd build
ctest --output-on-failure
# O ejecutar el binario directamente:
./bin/mnemis_tests
```
