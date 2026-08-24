# UI Controllers

Controllers expose application services to QML.

The controller layer:

- Translates C++ domain models to QVariant data.
- Owns UI navigation state.
- Does not access SQLite directly.
- Does not perform filesystem operations directly.
- Uses LibraryService as its only library-facing dependency.
