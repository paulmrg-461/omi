#include "opus_encoder.h"

#ifndef DISABLE_OPUS
#include <opus.h>
#include <esp_heap_caps.h>

#include "config.h"

// Opus encoder instance
static OpusEncoder *encoder = nullptr;
static opus_encoded_handler encoded_callback = nullptr;

// Ring buffer for PCM data - allocated in PSRAM
static int16_t *pcm_ring_buffer = nullptr;
static volatile size_t ring_write_pos = 0;
static volatile size_t ring_read_pos = 0;

// Output buffer - allocated in PSRAM
static uint8_t *opus_output_buffer = nullptr;
static int16_t *opus_input_buffer = nullptr;

bool opus_encoder_init()
{
    if (encoder != nullptr) {
        Serial.println("Opus encoder already initialized");
        return true;
    }

    Serial.println("Initializing Opus encoder...");

    // Allocate buffers in PSRAM
    pcm_ring_buffer = (int16_t *)heap_caps_malloc(AUDIO_RING_BUFFER_SAMPLES * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    if (pcm_ring_buffer == nullptr) {
        Serial.println("Failed to allocate PCM ring buffer in PSRAM");
        return false;
    }
    Serial.println("PCM ring buffer allocated in PSRAM");

    opus_output_buffer = (uint8_t *)heap_caps_malloc(OPUS_OUTPUT_MAX_BYTES, MALLOC_CAP_SPIRAM);
    if (opus_output_buffer == nullptr) {
        Serial.println("Failed to allocate opus output buffer in PSRAM");
        heap_caps_free(pcm_ring_buffer);
        pcm_ring_buffer = nullptr;
        return false;
    }

    opus_input_buffer = (int16_t *)heap_caps_malloc(OPUS_FRAME_SAMPLES * sizeof(int16_t), MALLOC_CAP_SPIRAM);
    if (opus_input_buffer == nullptr) {
        Serial.println("Failed to allocate opus input buffer in PSRAM");
        heap_caps_free(pcm_ring_buffer);
        heap_caps_free(opus_output_buffer);
        pcm_ring_buffer = nullptr;
        opus_output_buffer = nullptr;
        return false;
    }

    int error;
    encoder = opus_encoder_create(MIC_SAMPLE_RATE, 1, OPUS_APPLICATION_VOIP, &error);

    if (error != OPUS_OK || encoder == nullptr) {
        Serial.printf("Failed to create Opus encoder: %d\n", error);
        heap_caps_free(pcm_ring_buffer);
        heap_caps_free(opus_output_buffer);
        heap_caps_free(opus_input_buffer);
        pcm_ring_buffer = nullptr;
        opus_output_buffer = nullptr;
        opus_input_buffer = nullptr;
        return false;
    }

    // Configure encoder for voice
    opus_encoder_ctl(encoder, OPUS_SET_BITRATE(OPUS_BITRATE));
    opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(OPUS_COMPLEXITY));
    opus_encoder_ctl(encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    opus_encoder_ctl(encoder, OPUS_SET_VBR(OPUS_VBR));
    opus_encoder_ctl(encoder, OPUS_SET_VBR_CONSTRAINT(0));
    opus_encoder_ctl(encoder, OPUS_SET_LSB_DEPTH(16));
    opus_encoder_ctl(encoder, OPUS_SET_DTX(0));
    opus_encoder_ctl(encoder, OPUS_SET_INBAND_FEC(0));
    opus_encoder_ctl(encoder, OPUS_SET_PACKET_LOSS_PERC(0));

    // Reset ring buffer
    ring_write_pos = 0;
    ring_read_pos = 0;

    Serial.println("Opus encoder initialized successfully");
    Serial.printf("  Sample rate: %d Hz\n", MIC_SAMPLE_RATE);
    Serial.printf("  Bitrate: %d bps\n", OPUS_BITRATE);
    Serial.printf("  Frame size: %d samples (%d ms)\n", OPUS_FRAME_SAMPLES, OPUS_FRAME_SAMPLES * 1000 / MIC_SAMPLE_RATE);

    return true;
}

void opus_set_callback(opus_encoded_handler callback)
{
    encoded_callback = callback;
}

int opus_receive_pcm(int16_t *data, size_t samples)
{
    if (pcm_ring_buffer == nullptr) {
        return -1;
    }

    // Write to ring buffer
    for (size_t i = 0; i < samples; i++) {
        pcm_ring_buffer[ring_write_pos] = data[i];
        ring_write_pos = (ring_write_pos + 1) % AUDIO_RING_BUFFER_SAMPLES;
        
        // If we wrapped around and hit read pos, advance read pos (drop oldest data)
        if (ring_write_pos == ring_read_pos) {
            ring_read_pos = (ring_read_pos + 1) % AUDIO_RING_BUFFER_SAMPLES;
        }
    }
    
    return 0;
}

void opus_process()
{
    if (encoder == nullptr || pcm_ring_buffer == nullptr) {
        return;
    }
    
    // Check available samples
    size_t available_samples;
    if (ring_write_pos >= ring_read_pos) {
        available_samples = ring_write_pos - ring_read_pos;
    } else {
        available_samples = AUDIO_RING_BUFFER_SAMPLES - ring_read_pos + ring_write_pos;
    }
    
    // Process frames if we have enough data
    while (available_samples >= OPUS_FRAME_SAMPLES) {
        // Read one frame from ring buffer
        for (int i = 0; i < OPUS_FRAME_SAMPLES; i++) {
            opus_input_buffer[i] = pcm_ring_buffer[ring_read_pos];
            ring_read_pos = (ring_read_pos + 1) % AUDIO_RING_BUFFER_SAMPLES;
        }
        
        available_samples -= OPUS_FRAME_SAMPLES;
        
        // Encode
        int len = opus_encode(encoder, opus_input_buffer, OPUS_FRAME_SAMPLES, opus_output_buffer, OPUS_OUTPUT_MAX_BYTES);
        
        if (len > 0) {
            // Send encoded data
            if (encoded_callback != nullptr) {
                encoded_callback(opus_output_buffer, len);
            }
        } else if (len < 0) {
            Serial.printf("Opus encode error: %d\n", len);
        }
    }
}

uint8_t opus_get_codec_id()
{
    return 20; // Opus
}

#else // DISABLE_OPUS

bool opus_encoder_init() {
    Serial.println("Opus encoder DISABLED");
    return true; 
}

void opus_set_callback(opus_encoded_handler callback) { (void)callback; }

int opus_receive_pcm(int16_t *data, size_t samples) { 
    (void)data; 
    (void)samples; 
    return 0; 
}

void opus_process() {}

uint8_t opus_get_codec_id() { return 0; }

#endif // DISABLE_OPUS
