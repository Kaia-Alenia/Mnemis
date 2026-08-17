# Third-Party Licenses

Mnemis utilizes the following third-party libraries for its Media Indexer and core functionality. We are committed to complying with open-source licenses and properly attributing the authors.

## FFmpeg
- **Usage:** Video and Audio metadata extraction.
- **License:** **LGPL v2.1+**
- **Configuration Policy:** Mnemis uses FFmpeg strictly as a dynamically linked library through its C API (`libavformat`, `libavcodec`, `libavutil`). We do **not** enable `--enable-gpl` or any non-free components during the build process to ensure the resulting binaries and the integration remain compliant with the LGPL.

## TagLib
- **Usage:** Audio metadata (ID3, Vorbis Comments, FLAC tags, etc.) extraction.
- **License:** **LGPL / MPL**
- **Configuration Policy:** TagLib is dynamically linked. Mnemis complies with the LGPL/MPL requirements by not modifying TagLib's source code and using it through its public API.

## stb_image
- **Usage:** Image dimensions and basic header parsing.
- **License:** **MIT / Public Domain**
- **Configuration Policy:** Included as a header-only library (`stb_image.h`) in our `src/third_party/stb` directory. 
- **Copyright:** (c) 2017 Sean Barrett
