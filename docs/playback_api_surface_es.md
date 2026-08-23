# Superficie API de Reproducción de Mnemis

<!-- HADS: ES Document -->

Este documento proporciona una descripción técnica general de la nueva superficie API de reproducción introducida en la Fase 14.

## Interfaces Principales

### `IPlaybackBackend`
La interfaz de abstracción del backend. Esta interfaz aísla la lógica principal del motor de reproducción específico utilizado (ej. LibMPV).

- `virtual void load(const QString& path) = 0;`
- `virtual void play() = 0;`
- `virtual void pause() = 0;`
- `virtual void stop() = 0;`
- `virtual void seek(qint64 positionMs) = 0;`
- `virtual void setVolume(int volume) = 0;`
- `virtual void setMuted(bool muted) = 0;`

**Señales (vía callback/listener o señales QObject en implementaciones):**
- Cambios de posición
- Cambios de duración
- Cambios de estado (Reproduciendo, Pausado, Detenido, Error)
- Reporte de errores

### `PlaybackEngine`
El gestor central del estado y las operaciones de reproducción. Transfiere los comandos al backend activo y mantiene una máquina de estados estricta (`Inactivo` -> `Cargando` -> `Listo`).

**Seguridad de Hilos:**
Utiliza `std::recursive_mutex` para permitir callbacks síncronos seguros desde los backends (ej. cuando un backend dispara un evento en el mismo hilo durante una transición de estado).

**Métodos Clave:**
- `void setBackend(std::unique_ptr<IPlaybackBackend> backend)`
- `void load(const QString& path)`
- `void play()`
- `void pause()`
- `void stop()`

**Propiedades Clave / Señales:**
- `PlaybackState state() const`
- `qint64 position() const`
- `qint64 duration() const`
- `QString errorString() const`

## Implementaciones

### `LibMpvBackend`
El backend de producción utilizando `libmpv`.

**Restricciones Importantes:**
- **Región:** Requiere `LC_NUMERIC="C"`. `libmpv` es notoriamente sensible a las configuraciones regionales.
- **Bucle de Eventos:** Expone un mecanismo de callback de reactivación para integrar el bucle `mpv_wait_event` en el bucle de eventos de Qt a través de `mpv_set_wakeup_cb`.

### `FakePlaybackBackend`
Un backend simulado de pruebas utilizado en pruebas unitarias para simular cambios de estado de reproducción sin invocar `libmpv`. Soporta el disparo síncrono de eventos para validar la reentrada y las restricciones de la máquina de estados de `PlaybackEngine`.

## PlaybackController
La fachada orientada a QML (si aplica, heredando de `PlaybackEngine` o envolviéndolo) que proporciona las propiedades enlazables Q_PROPERTY.
