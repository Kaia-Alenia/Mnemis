# Indexer

The indexer connects:

Filesystem
    ->
Media Classifier
    ->
Media Repository
    ->
SQLite

Responsibilities:

- Traverse a configured root.
- Inspect files discovered by the filesystem layer.
- Classify supported media.
- Ignore unsupported files.
- Store media metadata in SQLite.
- Avoid duplicate rows by canonical path.

The indexer does not:

- Decode media.
- Generate thumbnails.
- Play media.
- Own UI state.
- Render folders.
