# Guía de Integración de Firmware - OMI Glasses (ESP32S3)

Este documento detalla los endpoints y protocolos necesarios para integrar el firmware de las gafas OMI (basado en ESP32S3) con el backend personalizado.

## Configuración General

*   **Protocolo:** HTTP/1.1 (REST) y WebSockets (WSS/WS)
*   **Host (Desarrollo):** `http://<IP_SERVIDOR>:8000`
*   **Host (Producción):** `https://api.tu-dominio.com`

---

## 1. Registro y Conexión de Dispositivo (Heartbeat)

Cada vez que las gafas se encienden o se conectan a WiFi, deben llamar a este endpoint para registrarse y obtener su `device_id` (UUID) interno, o confirmar que están en línea.

*   **Endpoint:** `/devices/connect`
*   **Método:** `POST`
*   **Content-Type:** `application/json`

### Cuerpo de la Petición (JSON)

| Campo | Tipo | Obligatorio | Descripción |
| :--- | :--- | :--- | :--- |
| `mac_address` | String | Sí | Dirección MAC del ESP32. Formato: `XX:XX:XX:XX:XX:XX` |
| `model` | String | Sí | Modelo del dispositivo (ej: `XIAO_ESP32S3`) |
| `firmware_version`| String | No | Versión actual del firmware (ej: `1.0.0`) |

**Ejemplo de Payload:**
```json
{
  "mac_address": "A0:B1:C2:D3:E4:F5",
  "model": "XIAO_ESP32S3",
  "firmware_version": "1.0.2"
}
```

### Respuesta Exitosa (200 OK)

El backend devolverá el objeto del dispositivo. **Es crucial guardar el campo `id`**, ya que se usará para abrir el socket de audio.

```json
{
  "mac_address": "A0:B1:C2:D3:E4:F5",
  "model": "XIAO_ESP32S3",
  "firmware_version": "1.0.2",
  "last_seen": "2023-10-27T10:00:00.000000",
  "status": "connected",
  "id": "550e8400-e29b-41d4-a716-446655440000" 
}
```

---

## 2. Streaming de Audio (WebSocket)

Para enviar audio en tiempo real al backend (para transcripción o almacenamiento), se utiliza una conexión WebSocket persistente.

*   **Endpoint:** `/ws/audio/stream/{device_id}`
*   **Protocolo:** `ws://` (o `wss://` en producción)
*   **Parámetro URL:** `{device_id}` es el UUID recibido en la respuesta del paso 1.

**URL Ejemplo:**
`ws://192.168.1.50:8000/ws/audio/stream/550e8400-e29b-41d4-a716-446655440000`

### Protocolo de Datos

1.  **Conexión:** Abrir el socket.
2.  **Envío de Audio:** El dispositivo debe enviar **datos binarios puros (Binary Frame)**.
    *   No enviar JSON ni texto por este socket.
    *   Formato recomendado: PCM Raw (16-bit, 16kHz o 44.1kHz, Mono).
    *   Tamaño de Chunk recomendado: 512 bytes - 4KB (ajustar según latencia deseada).
3.  **Keep-Alive:** Si no hay audio, el protocolo WebSocket maneja pings, pero se recomienda cerrar la conexión si no se va a transmitir por un tiempo prolongado para ahorrar batería.

### Ejemplo C++ (Arduino/ESP32)

Usando la librería `WebSocketsClient`:

```cpp
#include <WebSocketsClient.h>

WebSocketsClient webSocket;
String deviceId = "550e8400-e29b-41d4-a716-446655440000"; // Obtenido del paso 1

void setup() {
  // ... conexión WiFi ...
  
  // Iniciar WebSocket
  webSocket.begin("192.168.1.50", 8000, "/ws/audio/stream/" + deviceId);
  webSocket.onEvent(webSocketEvent);
  webSocket.setReconnectInterval(5000);
}

void loop() {
  webSocket.loop();
  
  if (audioAvailable) {
    // Leer buffer del micrófono I2S
    uint8_t * audioBuffer = readI2S(); 
    size_t audioSize = 1024;
    
    // Enviar binario
    webSocket.sendBIN(audioBuffer, audioSize);
  }
}
```

---

## 3. Flujo de Trabajo Sugerido (Firmware)

1.  **Inicio (Boot):** Inicializar hardware y conectar a WiFi.
2.  **Handshake:** 
    *   Hacer `POST /devices/connect` con la MAC address.
    *   Parsear el JSON de respuesta y guardar el `id` (UUID) en variable volátil.
3.  **Modo Espera (Idle):** Esperar comando de usuario (botón) o detección de voz (VAD local).
4.  **Transmisión:**
    *   Abrir conexión WebSocket a `/ws/audio/stream/{id}`.
    *   Comenzar a enviar buffers de audio capturados por el micrófono I2S.
    *   Mantener el envío mientras dure la interacción.
5.  **Finalización:** Cerrar WebSocket al terminar la interacción para liberar recursos.

---

## Códigos de Error Comunes

*   **422 Validation Error:** El formato de la MAC address es incorrecto (debe ser XX:XX:XX:XX:XX:XX) o faltan campos obligatorios.
*   **400 Bad Request (WS):** Intentar conectar al socket sin un ID válido.
*   **1000 Normal Closure (WS):** El servidor cerró la conexión correctamente.
