# Media Classifier

Classifies media by file extension.

Supported categories:

- Image
- Video
- Audio
- Unknown

The classifier is intentionally independent of:

- SQLite
- Filesystem scanning
- UI
- Playback
- FFmpeg

Actual media decoding/validation belongs to later layers.
