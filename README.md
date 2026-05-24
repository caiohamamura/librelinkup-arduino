# LibreLinkUp Arduino

Arduino/PlatformIO client for reading the latest LibreLinkUp glucose value on
ESP32 and ESP8266.

This is an unofficial client for the LibreView/LibreLinkUp API. The API can
change without notice.

## Supported Boards

- ESP32 Arduino core
- ESP8266 Arduino core

## PlatformIO Usage

Install from the PlatformIO Registry:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
lib_deps =
    caiohamamura/LibreLinkUp @ ^0.2.0
```

For ESP8266:

```ini
[env:nodemcuv2]
platform = espressif8266
board = nodemcuv2
framework = arduino
monitor_speed = 115200
lib_deps =
    caiohamamura/LibreLinkUp @ ^0.2.0
```

`ArduinoJson` is declared as a library dependency and is installed
automatically by PlatformIO.

## Basic Cached Loop Example

```cpp
#include <LibreLinkUp.h>

LibreLinkUpClient libre(
    "your-email@example.com",
    "your-password",
    "https://api.libreview.io"
);

void onReading(const LibreLinkUpReading& reading) {
    Serial.print("Patient: ");
    Serial.println(reading.patientName);
    Serial.print("Glucose: ");
    Serial.print(reading.valueMgDl);
    Serial.println(" mg/dL");
}

void onError(const String& error) {
    Serial.println(error);
}

void setup() {
    Serial.begin(115200);
    // Connect WiFi before calling libre.loop().
    libre.onUpdate(onReading);
    libre.onError(onError);
    libre.setup(); // default: connection 0, cache for 60 seconds
}

void loop() {
    libre.loop(); // refreshes only when cache is older than 1 minute

    if (libre.hasReading()) {
        Serial.println(libre.patientName());
        Serial.println(libre.valueMgDl());
    }
}
```

The low-level synchronous call is still available when you need full control:

```cpp
LibreLinkUpReading reading;
if (libre.getLatestReading(reading)) {
    Serial.println(reading.valueMgDl);
}
```

`LibreLinkUpReading` keeps the raw API values in `trendArrow` and
`measurementColor`, and also exposes semantic enums in `trend` and `color`.

Trend enum mapping:

| Raw | Enum | Name |
|---:|---|---|
| 1 | `LibreLinkUpTrendArrow::DownFast` | `down_fast` |
| 2 | `LibreLinkUpTrendArrow::DownSlow` | `down_slow` |
| 3 | `LibreLinkUpTrendArrow::Stable` | `stable` |
| 4 | `LibreLinkUpTrendArrow::UpSlow` | `up_slow` |
| 5 | `LibreLinkUpTrendArrow::UpFast` | `up_fast` |

Measurement color enum mapping:

| Raw | Enum | Name |
|---:|---|---|
| 1 | `LibreLinkUpMeasurementColor::InTarget` | `in_target` |
| 2 | `LibreLinkUpMeasurementColor::High` | `high` |
| 3 | `LibreLinkUpMeasurementColor::VeryHigh` | `very_high` |
| 4 | `LibreLinkUpMeasurementColor::Low` | `low` |

Complete examples:

- `examples/esp32_latest_reading`
- `examples/esp8266_latest_reading`

## API Notes

- `setup()` configures the cached loop API.
- `loop()` reuses the cached reading until the cache is older than 60 seconds.
- `updateNow()` forces a refresh immediately.
- `patientName()`, `timestamp()`, `valueMgDl()`, `trendName()`, and
  `colorName()` read from the latest cached value.
- The public API is loop/callback based. The HTTPS refresh still uses the
  built-in secure HTTP clients internally because portable async HTTPS is not
  reliable across both ESP32 and ESP8266.
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
