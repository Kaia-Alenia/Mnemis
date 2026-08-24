CREATE TABLE IF NOT EXISTS playlists (
    playlist_id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    created_time INTEGER NOT NULL,
    modified_time INTEGER NOT NULL,
    is_smart INTEGER NOT NULL DEFAULT 0,
    query_json TEXT
);

CREATE TABLE IF NOT EXISTS playlist_items (
    playlist_id TEXT NOT NULL,
    media_id TEXT NOT NULL,
    position INTEGER NOT NULL,
    added_time INTEGER NOT NULL,
    PRIMARY KEY (playlist_id, position),
    FOREIGN KEY(playlist_id) REFERENCES playlists(playlist_id) ON DELETE CASCADE,
    FOREIGN KEY(media_id) REFERENCES media(media_id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_playlist_items_media_id ON playlist_items(media_id);
