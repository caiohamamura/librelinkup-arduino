#include <Arduino.h>
#include <unity.h>

#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include "LibreLinkUp.h"
#include "../test_config.h"

LibreLinkUpClient client(
    TEST_LIBRE_EMAIL,
    TEST_LIBRE_PASSWORD,
    TEST_LIBRE_BASE_URL,
    "4.16.0"
);

void connect_wifi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(TEST_WIFI_SSID, TEST_WIFI_PASSWORD);

    unsigned long started = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - started < 30000) {
        delay(250);
    }

    TEST_ASSERT_EQUAL(WL_CONNECTED, WiFi.status());
}

void test_login_sets_auth_token_and_account_id() {
    bool ok = client.login();

    if (!ok) {
        TEST_FAIL_MESSAGE(client.lastError().c_str());
    }

    TEST_ASSERT_TRUE(client.authToken().length() > 20);
    TEST_ASSERT_EQUAL(64, client.accountId().length());
}

void test_get_latest_reading() {
    LibreLinkUpReading reading;

    bool ok = client.getLatestReading(reading);

    if (!ok) {
        TEST_FAIL_MESSAGE(client.lastError().c_str());
    }

    TEST_ASSERT_TRUE(reading.patientName.length() > 0);
    TEST_ASSERT_TRUE(reading.timestamp.length() > 0);
    TEST_ASSERT_TRUE(reading.valueMgDl > 0);
    TEST_ASSERT_TRUE(reading.value > 0.0);
}

void setup() {
    delay(1000);

    UNITY_BEGIN();

    RUN_TEST(connect_wifi);
    RUN_TEST(test_login_sets_auth_token_and_account_id);
    RUN_TEST(test_get_latest_reading);

    UNITY_END();
}

void loop() {
    delay(10);
}
