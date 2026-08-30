from pathlib import Path

p = Path("src/ui/controllers/LibraryController.cpp")
text = p.read_text()

old = '''    const QString mediaPath =
        QString::fromStdString(
            media.path
        );
'''

new = '''    const QString mediaPath =
        QString::fromStdString(
            media.canonicalPath
        );
'''

if old not in text:
    raise SystemExit("No se encontró media.path")

text = text.replace(old, new, 1)
p.write_text(text)

print("media.path -> media.canonicalPath corregido.")
