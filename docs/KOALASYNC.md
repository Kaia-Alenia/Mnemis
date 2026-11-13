# KoalaSync in Mnemis

## KoalaSync en Mnemis

Mnemis connects to KoalaSync through a native OkHttp WebSocket client using Engine.IO v4 and Socket.IO framing. The current implementation joins rooms, reports peer and playback state, and applies remote control events.

Mnemis se conecta a KoalaSync mediante un cliente WebSocket nativo de OkHttp usando tramas Engine.IO v4 y Socket.IO. La implementación actual entra en salas, informa del estado de participantes y reproducción, y aplica eventos de control remoto.

## Room lifecycle / Ciclo de una sala

1. Generate a local peer ID.
2. Open the configured secure WebSocket endpoint.
3. Complete the Engine.IO and Socket.IO handshake.
4. Send `join_room` with room ID and optional metadata.
5. Exchange room state, playback actions, and peer status.
6. Send `leave_room` and close the connection when finished.

1. Generar un peer ID local.
2. Abrir el endpoint WebSocket seguro configurado.
3. Completar el handshake de Engine.IO y Socket.IO.
4. Enviar `join_room` con el ID y metadatos opcionales.
5. Intercambiar estado de sala, acciones de reproducción y estado de participantes.
6. Enviar `leave_room` y cerrar la conexión al terminar.

## Events / Eventos

The implementation currently handles room data, errors, control mode, play, pause, seek, peer status, heartbeat, reconnect, and drift correction events. Playback positions are bounded to one day and messages carry sequence numbers to reduce stale updates.

La implementación gestiona datos de sala, errores, modo de control, play, pause, seek, estado de participantes, heartbeat, reconexión y corrección de desfase. Las posiciones se limitan a un día y los mensajes llevan números de secuencia para reducir actualizaciones obsoletas.

## Web playback

For HTML5 video sources, `KoalaSyncWebBridge` connects JavaScript video state to the native client. WebView integrations must validate navigation, avoid exposing privileged native methods, and never inject secrets into page content.

Para fuentes de video HTML5, `KoalaSyncWebBridge` conecta el estado JavaScript con el cliente nativo. Las integraciones WebView deben validar la navegación, evitar exponer métodos nativos privilegiados y nunca inyectar secretos en el contenido de la página.

## Compatibility

The KoalaSync protocol and server behavior may change. Keep protocol changes covered by unit tests and document changes to event names, payload fields, control modes, or reconnection behavior.

El protocolo y el servidor de KoalaSync pueden cambiar. Mantén los cambios cubiertos por pruebas unitarias y documenta cualquier modificación de eventos, campos, modos de control o reconexión.
