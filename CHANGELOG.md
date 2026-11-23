# Changelog de Mnemis

Todos los cambios notables en este proyecto se documentan en este archivo.
El formato se basa en [Keep a Changelog](https://keepachangelog.com/es-ES/1.1.0/) y este proyecto se adhiere a [Semantic Versioning](https://semver.org/lang/es/).

---

## [v1.0.9] - 2026-11-22

### Resumen de la Versión
Esta versión marca un hito fundamental en Mnemis: la integración del plugin unificado **Alenia Fuse**, la resolución integral del **Live Chat E2E**, la sincronización perfecta **Mnemis-to-Mnemis** en KoalaSync, la finalización al 100% de la experiencia de **YouTube**, la estabilización de **Picture-in-Picture (PiP)** sin crasheos y la habilitación de **reproducción en segundo plano con pantalla apagada** en Android 14.

---

### Novedades y Mejoras Principales

#### 1. Live Chat en Vivo E2E (KoalaSync) — 100% Operativo
- **Derivación Determinista de Clave por Sala**: Implementado `deriveDefaultSecret(roomId)` mediante SHA-256 HKDF. Ahora dos o más personas que se unan a la misma sala sin ingresar una clave manual comparten automáticamente la misma clave de cifrado E2E, eliminando el fallo de descifrado silencioso.
- **Soporte de Claves Privadas**: Si los participantes definen una clave personalizada de 16 bytes en base64url, la sala queda restringida exclusivamente a quienes la conozcan.
- **Codificación Canónica Base64URL RFC 4648**: Reescribiendo el codificador y decodificador para admitir con precisión todos los residuos de bytes (0, 1, 2) sin excepciones de límites de índice ni incompatibilidades con el relay server en Node.js.
- **Eco Local Optimista Inmediato**: Los mensajes enviados por el usuario aparecen al instante en su propia interfaz con timestamp y confirmación visual, deduplicando el eco devuelto por el WebSocket.

#### 2. Sincronización Mnemis-to-Mnemis — Sincronización Perfecta
- **Preservación de Estado y Desfase**: Sincronización continua de Play, Pause y Seek con filtro adaptativo de latencia RTT (EMA) y corrección suave de desvío (drift check).
- **Mapeo de Presencia y Roles**: Roles de anfitrión y participantes sincronizados con reconexión exponencial ante pérdidas momentáneas de cobertura.

#### 3. YouTube — 100% Terminado
- **Pre-resolución de Stream Directo InnerTube**: Detección y resolución asíncrona de streams directos de YouTube en segundo plano mientras el video corre en el WebView.
- **Traspaso Fluido a Reproductor Nativo**: Traspaso seguro con banderas `FLAG_ACTIVITY_NEW_TASK or FLAG_ACTIVITY_CLEAR_TOP`, preservando la posición al milisegundo y re-conectando de inmediato a la misma sala de KoalaSync.
- **Bloqueo Temprano de Anuncios**: Inyección de reglas CSS y bloqueo de peticiones de red antes de la carga de anuncios pre-roll de YouTube.

#### 4. Estado de Plataformas de Streaming
- **YouTube**: Finalizado y probado al 100%.
- **Otras Plataformas**: El Hub de Streaming incluye lanzadores e intents para **Twitch**, **Crunchyroll**, **Kick**, **Dailymotion** y **Jellyfin**, además del botón sniffer de streams HLS/DASH. Las pruebas y optimizaciones específicas para estas plataformas quedan programadas para las próximas iteraciones.

#### 5. Plugin Unificado Alenia Fuse
- **Integración de Fuse PC (`Alenia-Porter`) en Android**:
  - **Inspección Técnica (`/info`)**: Lectura de codecs de video/audio, resolución, bitrate, duración, contenedor y estimación de tasa de transferencia.
  - **Compresión Inteligente (`/compress`)**: Perfiles optimizados para mensajería (WhatsApp 16 MB, Discord 25 MB, Email 10 MB, Web 720p) y modo manual por tamaño objetivo en MB con cálculo automático de bitrate de video y audio.
  - **Conversión y Transcodificación por Hardware (`/convert`)**: Motor basado en AndroidX Media3 Transformer con codecs de hardware H.264/AAC.
  - **Remuxing Ultrarrápido (`/remux`)**: Remux sin pérdida de calidad mediante MediaMuxer en contenedores MP4 y MKV.
  - **Extracción de Audio y Recorte**: Exportación directa de pistas de audio y fragmentación temporal de clips.
- **Interfaz Integrada**: Diálogo modal accesible desde la biblioteca de videos (botón Fuse y pulsación prolongada) y desde el reproductor activo.

#### 6. Picture-in-Picture (PiP) Fluido y Seguro
- **Aspect Ratio Dinámico Delimitado**: Implementado `calculateSafeAspectRatio()` que acota las dimensiones entre `0.42` (1:2.39) y `2.38` (2.39:1). Previene el crasheo `IllegalArgumentException` en videos verticales (Shorts/Reels) o ultra-panorámicos.
- **Transición Nativa en Android 12+ (API 31+)**: Configurado `setAutoEnterEnabled(true)` y `setSeamlessResizeEnabled(true)` para transiciones con gestos del sistema sin congelamiento de superficie.
- **Limpieza de Capas y Restauración**: Desactivación del interceptor de gestos `gestureTouchView` durante PiP y redibujado forzado al volver a pantalla completa.

#### 7. Reproducción en Segundo Plano y Pantalla Apagada
- **Compatibilidad con Android 14 (`targetSdk = 34`)**: Actualizado `PlaybackService` para registrar `ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK` en `startForeground()`.
- **Foco de Audio y Wake Lock**: ExoPlayer configurado con `AudioAttributes` multimedia, gestión automática de foco de audio, `setHandleAudioBecomingNoisy(true)` y `setWakeMode(C.WAKE_MODE_NETWORK)`.
- **Controles en Barra de Notificaciones**: Controles interactivos de Play, Pause y Detener accesibles desde la barra de estado y la pantalla de bloqueo.
- **Desvinculación Segura de Superficie**: Desconexión temporal de `playerView.player = null` en `onStop()` cuando la pantalla se apaga o la app se minimiza sin PiP, permitiendo que la pista de audio continúe ininterrumpida y reconectándose de inmediato en `onStart()`.

#### 8. Cierre y Purga de Sesiones Fantasma
- **`peerId` Persistente por Dispositivo**: Almacenado en `MnemisPreferences`, garantizando que al reconectar o cambiar entre vistas el relay server expulse la sesión anterior inmediatamente por deduplicación.
- **Salida Explícita**: Envío de evento `LEAVE_ROOM` con `{ peerId }` antes de cerrar la conexión WebSocket.
- **Purga de Inactividad Local**: Monitoreo de `peerLastSeen` que purga automáticamente de la sala a cualquier usuario que no envíe señales de vida en más de 45 segundos.

---

## [v1.0.8] - 2026-11-22
- Inicialización de integración de AndroidX Media3 Transformer.
- Adición de intents `ACTION_SEND` para enlaces compartidos desde apps externas.
- Streaming Hub inicial con accesos directos a plataformas de video.

## [v1.0.7] - 2026-11-21
- Traspaso inicial de YouTube a reproductor nativo.
- Mejoras en control de orientación y modo de relación de aspecto.

## [v1.0.0 - v1.0.6]
- Núcleo de reproducción extensible con arquitectura basada en plugins.
- Salas nativas de sincronización KoalaSync con WebSockets.
- Soporte para HLS (`.m3u8`), DASH y archivos locales.
- Gestor de plugins dinámicos y bloqueador de publicidad web.
