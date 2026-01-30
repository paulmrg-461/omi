#include "audio_output.h"
#include "config.h"
#include <math.h>

#define I2S_SPEAKER_PORT I2S_NUM_1
#define SAMPLE_RATE 16000 // Match mic sample rate for simplicity

static bool speaker_initialized = false;

bool audio_output_init() {
    if (speaker_initialized) return true;

    Serial.println("Initializing I2S Speaker...");
    Serial.printf("  LRC Pin: %d\n", I2S_SPEAKER_LRC);
    Serial.printf("  BCLK Pin: %d\n", I2S_SPEAKER_BCLK);
    Serial.printf("  DIN Pin: %d\n", I2S_SPEAKER_DIN);

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // Standard for MAX98357A
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SPEAKER_BCLK,
        .ws_io_num = I2S_SPEAKER_LRC,
        .data_out_num = I2S_SPEAKER_DIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    esp_err_t err = i2s_driver_install(I2S_SPEAKER_PORT, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("Failed to install I2S speaker driver: %d\n", err);
        return false;
    }

    err = i2s_set_pin(I2S_SPEAKER_PORT, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("Failed to set I2S speaker pins: %d\n", err);
        i2s_driver_uninstall(I2S_SPEAKER_PORT);
        return false;
    }

    i2s_zero_dma_buffer(I2S_SPEAKER_PORT);
    speaker_initialized = true;
    Serial.println("Audio output initialized successfully");
    return true;
}

void audio_output_write(int16_t *samples, size_t count) {
    if (!speaker_initialized) return;
    
    size_t bytes_written;
    i2s_write(I2S_SPEAKER_PORT, samples, count * sizeof(int16_t), &bytes_written, portMAX_DELAY);
}

void audio_output_beep(int freq_hz, int duration_ms) {
    if (!speaker_initialized) return;

    size_t samples_count = (SAMPLE_RATE * duration_ms) / 1000;
    // Limit allocation to avoid crash
    if (samples_count > 16000) samples_count = 16000; // Max 1 sec buffer at a time

    int16_t *samples = (int16_t *)malloc(samples_count * sizeof(int16_t));
    if (!samples) {
        Serial.println("Failed to allocate beep buffer");
        return;
    }

    for (size_t i = 0; i < samples_count; i++) {
        // Sine wave generated at 50% volume (10000 amplitude)
        samples[i] = (int16_t)(10000.0 * sin(2.0 * PI * freq_hz * i / SAMPLE_RATE));
    }

    audio_output_write(samples, samples_count);
    free(samples);
}

void audio_output_stop() {
    if (speaker_initialized) {
        i2s_driver_uninstall(I2S_SPEAKER_PORT);
        speaker_initialized = false;
    }
}
