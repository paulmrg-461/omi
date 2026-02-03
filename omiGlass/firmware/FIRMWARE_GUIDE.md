# Guía de Integración de Firmware - OMI Glasses (ESP32S3)

Este documento detalla los endpoints y protocolos necesarios para integrar el firmware de las gafas OMI (basado en ESP32S3) con el backend personalizado.

## Configuración Actual (Standalone Mode)

*   **Modo:** Standalone (Sin App Móvil, WiFi Directo)
*   **WiFi SSID:** `FLIA_REALPE`
*   **Host API:** `pq48nm3b-1719.use2.devtunnels.ms` (Puerto 443 -> 1719)
*   **Funcionalidad:** Captura de fotos cada 60 segundos (Audio desactivado)

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
| `firmware_version`| String | No | Versión actual del firmware (ej: `2.1.1`) |

**Ejemplo de Payload:**
```json
{
  "mac_address": "A0:B1:C2:D3:E4:F5",
  "model": "XIAO_ESP32S3",
  "firmware_version": "2.1.1"
}
```

### Respuesta Exitosa (200 OK)

```json
{
  "mac_address": "A0:B1:C2:D3:E4:F5",
  "model": "XIAO_ESP32S3",
  "firmware_version": "2.1.1",
  "last_seen": "2023-10-27T10:00:00.000000",
  "status": "connected",
  "id": 1
}
```

---

## 2. Análisis de Visión (Cámara) - ACTIVO

Este endpoint permite enviar una foto tomada por las gafas para ser analizada por la IA (Gemini). El resultado se imprimirá en la consola del servidor.

**Frecuencia:** Cada 60 segundos.

*   **Endpoint:** `/vision/analyze`
*   **Método:** `POST`
*   **Content-Type:** `multipart/form-data`

### Parámetros del Formulario (Multipart)

| Campo | Tipo | Obligatorio | Descripción |
| :--- | :--- | :--- | :--- |
| `file` | File (Binary) | Sí | La imagen capturada (JPG). |
| `prompt` | String | No | Instrucción para la IA (Defecto: "Describe this image in detail."). |

### Implementación Firmware (Standalone)

El firmware utiliza `WiFiClientSecure` para enviar la imagen en chunks multipart para evitar problemas de memoria.

```cpp
// Ejemplo simplificado
WiFiClientSecure client;
client.setInsecure();
if (client.connect("pq48nm3b-1719.use2.devtunnels.ms", 443)) {
    // ... Enviar headers y chunks de imagen ...
}
```

### Respuesta Exitosa (200 OK)

```json
{
  "analysis": "Una imagen que muestra un escritorio desordenado con cables y componentes electrónicos..."
}
```

---

## 3. Streaming de Audio (WebSocket) - SUSPENDIDO

**NOTA:** El streaming de audio está actualmente **DESHABILITADO** en el firmware para priorizar la transmisión de fotos.

*   **Endpoint:** `/ws/audio/stream/{device_id}`
*   **Protocolo:** `wss://`
*   **Estado:** Código comentado en `standalone.h`.

---

## Comandos Útiles

*   **Monitor Serial:** `python -m platformio device monitor`
*   **Subir Firmware:** `python -m platformio run --target upload`
