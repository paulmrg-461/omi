#ifndef AUDIO_OUTPUT_H
#define AUDIO_OUTPUT_H

#include <Arduino.h>
#include <driver/i2s.h>

// Initialize the I2S speaker output
bool audio_output_init();

// Play a raw PCM buffer (16-bit signed, mono)
// Note: This is blocking or non-blocking depending on implementation, 
// usually blocking until DMA buffer is full.
void audio_output_write(int16_t *samples, size_t count);

// Generate a simple beep for testing
void audio_output_beep(int freq_hz, int duration_ms);

// Stop the I2S output
void audio_output_stop();

#endif // AUDIO_OUTPUT_H
