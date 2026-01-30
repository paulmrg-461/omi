# Guía de Hardware de Audio para Omi Glasses (Reproducción)

Actualmente, el firmware de Omi Glasses está enfocado en la captura de audio (micrófono) y video (cámara). Sin embargo, el **Seeed Studio XIAO ESP32S3 Sense** tiene pines disponibles para añadir capacidad de reproducción de audio (hablar desde las gafas).

## Hardware Recomendado

Para añadir audio (altavoz) a tus Omi Glasses, necesitas un **Amplificador I2S**. La opción más común, compacta y compatible es el **MAX98357A**.

### Lista de Compras
1.  **Módulo Amplificador I2S MAX98357A** (aprox. $3 - $6 USD).
2.  **Pequeño Altavoz / Speaker**:
    *   Impedancia: 4 Ohm u 8 Ohm.
    *   Potencia: 3W (o menos, para ahorrar batería).
    *   Tamaño: Lo más pequeño posible para montar en las gafas (ej. 20mm x 14mm ovalado, o "bone conduction transducer" pequeño).

## Conexión (Wiring)

El XIAO ESP32S3 Sense utiliza muchos pines para la cámara y el micrófono, pero los pines expuestos en los laterales (D0-D10) están mayormente libres.

**Diagrama de Conexión:**

| Pin MAX98357A | Pin XIAO ESP32S3 | Función | GPIO (Interno) |
| :--- | :--- | :--- | :--- |
| **LRC** | **D2** | Word Select (LRCK) | GPIO 3 |
| **BCLK** | **D3** | Bit Clock (BCLK) | GPIO 4 |
| **DIN** | **D4** | Data In | GPIO 5 |
| **GND** | **GND** | Tierra | - |
| **VIN** | **5V** o **BAT+** | Alimentación | - |

> **Nota sobre Alimentación (VIN):**
> *   Si usas USB: Conecta a **5V**.
> *   Si usas Batería: Conecta al **positivo de la batería (BAT+)** o a **3.3V** (aunque el volumen será menor en 3.3V). El MAX98357A funciona desde 2.5V hasta 5.5V.

## Cambios Necesarios en Firmware

Para que esto funcione, se debe actualizar el firmware para inicializar el segundo puerto I2S (`I2S_NUM_1`) para salida. El puerto 0 ya está usado por el micrófono.

### 1. Definir Pines en `config.h`

```cpp
// Audio Output (Speaker) - MAX98357A
#define SPEAKER_I2S_PORT I2S_NUM_1
#define SPEAKER_BCLK_PIN 3  // D2
#define SPEAKER_LRCK_PIN 4  // D3
#define SPEAKER_DOUT_PIN 5  // D4
```

### 2. Inicialización (Ejemplo)

```cpp
i2s_config_t tx_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = 16000, // Debe coincidir con el audio recibido
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false
};

i2s_pin_config_t tx_pin_config = {
    .bck_io_num = SPEAKER_BCLK_PIN,
    .ws_io_num = SPEAKER_LRCK_PIN,
    .data_out_num = SPEAKER_DOUT_PIN,
    .data_in_num = I2S_PIN_NO_CHANGE
};

i2s_driver_install(SPEAKER_I2S_PORT, &tx_config, 0, NULL);
i2s_set_pin(SPEAKER_I2S_PORT, &tx_pin_config);
```
