# LibreLinkUp Arduino

Arduino/PlatformIO client for reading the latest LibreLinkUp glucose value on
ESP32 and ESP8266.

This is an unofficial client for the LibreView/LibreLinkUp API. The API can
change without notice.

## Supported Boards

- ESP32 Arduino core
- ESP8266 Arduino core

## PlatformIO Usage

If this repository is next to your PlatformIO project, reference it with
`lib_extra_dirs`:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps =
    bblanchon/ArduinoJson @ ^7.0.0
lib_extra_dirs = ../librelinkup-arduino
```

For ESP8266:

```ini
[env:nodemcuv2]
platform = espressif8266
board = nodemcuv2
framework = arduino
monitor_speed = 115200
lib_deps =
    bblanchon/ArduinoJson @ ^7.0.0
lib_extra_dirs = ../librelinkup-arduino
```

## Basic Example

```cpp
#include <LibreLinkUp.h>

LibreLinkUpClient libre(
    "your-email@example.com",
    "your-password",
    "https://api.libreview.io"
);

LibreLinkUpReading reading;
if (libre.getLatestReading(reading)) {
    Serial.print("Patient: ");
    Serial.println(reading.patientName);
    Serial.print("Timestamp: ");
    Serial.println(reading.timestamp);
    Serial.print("Glucose: ");
    Serial.print(reading.valueMgDl);
    Serial.println(" mg/dL");
} else {
    Serial.println(libre.lastError());
}
```

Complete examples:

- `examples/esp32_latest_reading`
- `examples/esp8266_latest_reading`

## API Notes

- Default base URL: `https://api.libreview.io`
- Default client version: `4.16.0`
- The client logs in with `POST /llu/auth/login`
- The latest reading is read from `GET /llu/connections/{patientId}/graph`
- `Account-Id` is generated as SHA-256 of the returned user id
- The client requests `Accept-Encoding: identity`; do not switch this to `gzip`
  unless you add explicit decompression before JSON parsing.

## TLS Note

The current implementation uses `setInsecure()` for simple ESP32/ESP8266
prototyping. For production use, add proper LibreView root CA validation and
ensure the device clock is valid before making HTTPS requests.

## Build Checks

From each example directory:

```bash
pio run
```

Verified locally:

- `examples/esp32_latest_reading`
- `examples/esp8266_latest_reading`
