CREATE TABLE IF NOT EXISTS folders (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    library_root_id INTEGER NOT NULL,
    parent_id       INTEGER,
    path            TEXT NOT NULL,
    name            TEXT NOT NULL,
    is_root         INTEGER NOT NULL DEFAULT 0
                    CHECK (is_root IN (0, 1)),

    FOREIGN KEY (library_root_id)
        REFERENCES library_roots(id)
        ON DELETE CASCADE,

    FOREIGN KEY (parent_id)
        REFERENCES folders(id)
        ON DELETE CASCADE,

    UNIQUE(library_root_id, path)
);

CREATE INDEX IF NOT EXISTS idx_folders_root
    ON folders(library_root_id);

CREATE INDEX IF NOT EXISTS idx_folders_parent
    ON folders(library_root_id, parent_id);

CREATE INDEX IF NOT EXISTS idx_folders_path
    ON folders(library_root_id, path);
