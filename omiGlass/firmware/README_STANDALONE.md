# OMI Glass Standalone Firmware (WiFi)

This firmware version allows the Omi Glasses to function without a smartphone connection by sending recorded audio directly to an API via WiFi.

## Configuration

Before building, you **MUST** configure your WiFi credentials and API endpoint in `src/config.h`:

```cpp
// =============================================================================
// WIFI & API CONFIGURATION (STANDALONE MODE)
// =============================================================================
#define WIFI_SSID "YOUR_WIFI_SSID"      // Change this
#define WIFI_PASS "YOUR_WIFI_PASSWORD"  // Change this
#define API_ENDPOINT "http://your-server.com/api/upload" // Change this
```

## Usage

1. **Power On**: Press the button. The LED will blink.
2. **Record & Send**:
   - **Press and Hold** the button to record audio.
   - **Release** the button to stop recording and send the audio to the API.
   - **LED Feedback**:
     - **ON**: Recording.
     - **Fast Blinks**: Sending.
     - **3 Slow Blinks**: Success.
     - **5 Fast Blinks**: Error (WiFi or API).
3. **Power Off**: Long press (2+ seconds) to turn off.

## Building and Flashing

Use the standard build script:

```bash
# Build
./scripts/build_uf2.sh

# Flash
# Copy the generated .uf2 file to the ESP32S3 drive.
```
