# Licencias de Terceros

<!-- HADS: ES Document -->

Mnemis utiliza las siguientes bibliotecas de terceros para su Indexador Multimedia y funcionalidad principal. Nos comprometemos a cumplir con las licencias de código abierto y atribuir adecuadamente a los autores.

## FFmpeg
- **Uso:** Extracción de metadatos de video y audio.
- **Licencia:** **LGPL v2.1+**
- **Política de Configuración:** Mnemis utiliza FFmpeg estrictamente como una biblioteca enlazada dinámicamente a través de su API en C (`libavformat`, `libavcodec`, `libavutil`). **No** habilitamos `--enable-gpl` ni componentes no libres durante la compilación para asegurar el cumplimiento con LGPL.

## TagLib
- **Uso:** Extracción de metadatos de audio (ID3, Vorbis Comments, FLAC tags, etc.).
- **Licencia:** **LGPL / MPL**
- **Política de Configuración:** TagLib se enlaza dinámicamente. Mnemis cumple con los requisitos LGPL/MPL al no modificar el código fuente y utilizarlo a través de su API pública.

## stb_image
- **Uso:** Dimensiones de imagen y análisis básico de cabeceras.
- **Licencia:** **MIT / Dominio Público**
- **Política de Configuración:** Incluida como biblioteca de solo cabecera (`stb_image.h`) en nuestro directorio `src/third_party/stb`.
- **Copyright:** (c) 2017 Sean Barrett
