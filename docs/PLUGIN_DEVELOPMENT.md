# Mnemis plugin development

## Desarrollo de plugins de Mnemis

Mnemis is intended to support modular media-source and playback integrations. The repository currently contains native integration code, but it does not yet publish a stable external plugin SDK. Treat the interfaces in this document as design guidance, not a compatibility guarantee.

Mnemis está pensado para admitir integraciones modulares de fuentes y reproducción. El repositorio contiene integraciones nativas, pero todavía no publica un SDK externo estable. Trata estas interfaces como guía de diseño, no como una garantía de compatibilidad.

## Plugin responsibilities / Responsabilidades

A plugin should:

- Identify the source type it supports.
- Validate and normalize input URLs.
- Return title, duration, artwork, and a playable URI when available.
- Declare required permissions and network hosts.
- Expose clear errors and cancellation behavior.
- Avoid logging URLs containing credentials or personal data.
- Work with the core player instead of duplicating playback controls.

Un plugin debe:

- Identificar el tipo de fuente compatible.
- Validar y normalizar las URLs de entrada.
- Devolver título, duración, carátula y una URI reproducible cuando existan.
- Declarar permisos y hosts de red necesarios.
- Exponer errores y cancelación de forma clara.
- Evitar registrar URLs con credenciales o datos personales.
- Usar el reproductor central sin duplicar sus controles.

## Recommended flow / Flujo recomendado

```text
user input
  -> validation
  -> source resolver
  -> normalized media item
  -> Media3 or WebView playback
  -> optional KoalaSync snapshot/control
```

Do not assume that a resolved URL is permanent. Resolvers should use bounded timeouts, validate content types, and fail visibly when a provider changes.

No asumas que una URL resuelta es permanente. Los resolvers deben usar timeouts limitados, validar tipos de contenido y fallar de forma visible cuando cambie un proveedor.

## KoalaSync integration

For watch-party support, a plugin must be able to provide a stable media identity and a playback snapshot. KoalaSync sends control actions such as play, pause, and seek; it does not transfer the media bytes.

Para admitir salas, un plugin debe poder proporcionar una identidad estable del medio y un snapshot de reproducción. KoalaSync envía acciones como play, pause y seek; no transfiere los bytes del medio.

See [KOALASYNC.md](KOALASYNC.md) for the current protocol behavior.
