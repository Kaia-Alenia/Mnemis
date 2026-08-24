CREATE TABLE IF NOT EXISTS library_roots (
    id       INTEGER PRIMARY KEY AUTOINCREMENT,
    path     TEXT NOT NULL UNIQUE,
    name     TEXT NOT NULL,
    enabled  INTEGER NOT NULL DEFAULT 1
             CHECK (enabled IN (0, 1))
);

CREATE INDEX IF NOT EXISTS idx_library_roots_enabled
    ON library_roots(enabled);

CREATE INDEX IF NOT EXISTS idx_library_roots_name
    ON library_roots(name COLLATE NOCASE);
