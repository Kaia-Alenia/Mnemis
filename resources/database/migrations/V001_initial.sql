PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS media (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    canonical_path  TEXT NOT NULL UNIQUE,
    file_name       TEXT NOT NULL,
    extension       TEXT NOT NULL DEFAULT '',
    media_type      INTEGER NOT NULL DEFAULT 0,
    file_size       INTEGER NOT NULL DEFAULT 0,
    modified_time   INTEGER NOT NULL DEFAULT 0,
    favorite        INTEGER NOT NULL DEFAULT 0
                    CHECK (favorite IN (0, 1))
);

CREATE INDEX IF NOT EXISTS idx_media_canonical_path
    ON media(canonical_path);

CREATE INDEX IF NOT EXISTS idx_media_media_type
    ON media(media_type);

CREATE INDEX IF NOT EXISTS idx_media_favorite
    ON media(favorite);
