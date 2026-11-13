# Contributing to Mnemis

## Contribuir a Mnemis

Thank you for helping improve Mnemis. Contributions may include Kotlin/Java code, source integrations, KoalaSync improvements, tests, translations, accessibility, and documentation.

Gracias por ayudar a mejorar Mnemis. Puedes contribuir con código Kotlin/Java, integraciones de fuentes, mejoras de KoalaSync, pruebas, traducciones, accesibilidad y documentación.

## Before opening a change / Antes de abrir un cambio

1. Search existing issues and pull requests.
2. For large changes, describe the proposed design first.
3. Keep changes focused and avoid unrelated formatting.
4. Never include secrets, private media URLs, or copyrighted media in commits.

1. Busca issues y pull requests existentes.
2. Para cambios grandes, describe primero el diseño propuesto.
3. Mantén los cambios enfocados y evita formateo no relacionado.
4. Nunca incluyas secretos, URLs privadas de medios ni material protegido en commits.

## Development

Use JDK 17 and Android SDK 34. Run the smallest relevant checks before submitting:

```bash
./gradlew test
./gradlew assembleDebug
```

Document behavior that depends on an external platform. External services can change without notice, so integrations should fail clearly and avoid silent fallbacks.

Usa JDK 17 y Android SDK 34. Ejecuta las comprobaciones relevantes antes de enviar cambios. Documenta el comportamiento que dependa de plataformas externas y evita fallos silenciosos.

## Pull requests / Pull requests

Describe the user-visible behavior, affected sources, permissions, privacy implications, and validation performed. Include screenshots only when they contain no private information.

Describe el comportamiento visible para el usuario, las fuentes afectadas, permisos, implicaciones de privacidad y validaciones realizadas. Incluye capturas solo si no contienen información privada.

## Plugin contributions / Contribuciones de plugins

Follow [Plugin development](docs/PLUGIN_DEVELOPMENT.md). A plugin must explain its permissions, supported sources, failure behavior, data handling, and compatibility assumptions.

Sigue [Desarrollo de plugins](docs/PLUGIN_DEVELOPMENT.md). Un plugin debe explicar sus permisos, fuentes compatibles, manejo de errores, tratamiento de datos y supuestos de compatibilidad.
