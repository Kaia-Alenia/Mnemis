# Security policy

## Política de seguridad

Mnemis handles network media, WebView content, room credentials, and real-time connections. We take reports about vulnerabilities seriously.

Mnemis gestiona contenido multimedia de red, contenido WebView, credenciales de salas y conexiones en tiempo real. Tomamos en serio los reportes de vulnerabilidades.

## Reporting a vulnerability / Reportar una vulnerabilidad

Please do **not** open a public issue for an undisclosed vulnerability. Send a private report to the repository maintainers through the repository's private security reporting channel, if enabled. If that channel is unavailable, contact the maintainers through the official Alenia Studios project channel and reference “Mnemis security report”.

No abras un issue público para una vulnerabilidad que aún no ha sido corregida. Envía el reporte de forma privada mediante el canal de reportes de seguridad del repositorio, si está habilitado. Si no está disponible, contacta a los responsables por el canal oficial del proyecto Alenia Studios e indica “Mnemis security report”.

Include:

- Affected version, commit, or build.
- Android version and device/emulator details.
- Clear reproduction steps or a minimal proof of concept.
- Impact and possible attack scenario.
- Logs or screenshots after removing tokens, cookies, personal data, and private URLs.

Incluye:

- Versión, commit o build afectado.
- Versión de Android y datos del dispositivo/emulador.
- Pasos claros o una prueba de concepto mínima.
- Impacto y posible escenario de ataque.
- Logs o capturas después de eliminar tokens, cookies, datos personales y URLs privadas.

Please allow maintainers reasonable time to investigate and release a fix before public disclosure. Do not test against rooms, accounts, or media that you do not own or have permission to use.

## Security guidance / Recomendaciones

- Never commit API keys, room passwords, cookies, or access tokens.
- Treat URLs received from third parties as untrusted input.
- Keep Android, Media3, OkHttp, and other dependencies updated.
- Use HTTPS/WSS sources whenever available.
- Do not share private room details in issues or screenshots.

- Nunca subas claves API, contraseñas de salas, cookies ni tokens.
- Trata las URLs de terceros como entradas no confiables.
- Mantén actualizadas las dependencias de Android, Media3, OkHttp y demás.
- Usa fuentes HTTPS/WSS siempre que sea posible.
- No compartas datos privados de salas en issues o capturas.
