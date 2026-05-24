#include <Arduino.h>
#include <WiFi.h>
#include <LibreLinkUp.h>

const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

const char* LIBRE_EMAIL = "YOUR_LIBRE_LINK_UP_EMAIL";
const char* LIBRE_PASSWORD = "YOUR_LIBRE_LINK_UP_PASSWORD";

LibreLinkUpClient libre(LIBRE_EMAIL, LIBRE_PASSWORD);

void printReading(const LibreLinkUpReading& reading) {
    Serial.print("Patient: ");
    Serial.println(reading.patientName);
    Serial.print("Timestamp: ");
    Serial.println(reading.timestamp);
    Serial.print("Glucose: ");
    Serial.print(reading.valueMgDl);
    Serial.println(" mg/dL");
    Serial.print("Trend: ");
    Serial.print(libreLinkUpTrendArrowName(reading.trend));
    Serial.print(" (");
    Serial.print(libreLinkUpTrendArrowSymbol(reading.trend));
    Serial.println(")");
    Serial.print("Range: ");
    Serial.println(libreLinkUpMeasurementColorName(reading.color));
}

void printError(const String& error) {
    Serial.print("LibreLinkUp error: ");
    Serial.println(error);
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("WiFi connected");

    libre.onUpdate(printReading);
    libre.onError(printError);
    libre.setup();
}

void loop() {
    libre.loop();

    if (libre.hasReading()) {
        Serial.print("Cached patient: ");
        Serial.println(libre.patientName());
        delay(5000);
    }
}
