# Thumbnail Engine

The thumbnail engine generates and caches image previews.

Phase 16 integrates it into LibraryController.

Flow:

LibraryService
    ->
MediaQuery
    ->
Media
    ->
ThumbnailService
    ->
local PNG cache
    ->
thumbnailUrl
    ->
QML Image

Image thumbnails use:

- persistent cache
- SHA-256 cache keys
- modified time in cache key
- FastTransformation
- smooth=false in QML
- mipmap=false

Video and audio previews remain outside this phase.
