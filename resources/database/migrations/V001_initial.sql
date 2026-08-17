CREATE TABLE IF NOT EXISTS media (
    media_id TEXT PRIMARY KEY,
    path TEXT NOT NULL,
    canonical_path TEXT NOT NULL UNIQUE,
    file_name TEXT NOT NULL,
    extension TEXT NOT NULL,
    mime_type TEXT NOT NULL,
    media_type INTEGER NOT NULL,
    file_size INTEGER NOT NULL,
    modified_time INTEGER NOT NULL,
    created_time INTEGER NOT NULL,
    
    width INTEGER,
    height INTEGER,
    
    duration REAL,
    frame_rate REAL,
    
    audio_channels INTEGER,
    audio_sample_rate INTEGER,
    title TEXT,
    artist TEXT,
    album TEXT,
    album_artist TEXT,
    genre TEXT,
    track_number INTEGER,
    disc_number INTEGER,
    year INTEGER,
    
    has_thumbnail INTEGER NOT NULL DEFAULT 0,
    thumbnail_version INTEGER NOT NULL DEFAULT 0,
    favorite INTEGER NOT NULL DEFAULT 0,
    last_played INTEGER,
    play_count INTEGER NOT NULL DEFAULT 0,
    
    index_state INTEGER NOT NULL DEFAULT 0,
    error_state TEXT
);

CREATE INDEX IF NOT EXISTS idx_media_canonical_path ON media(canonical_path);
CREATE INDEX IF NOT EXISTS idx_media_media_type ON media(media_type);
CREATE INDEX IF NOT EXISTS idx_media_favorite ON media(favorite);
