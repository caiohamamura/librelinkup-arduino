#include "LibreLinkUp.h"

#include <ArduinoJson.h>

#if defined(ESP32)
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#elif defined(ESP8266)
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#else
#error "LibreLinkUp supports ESP8266 and ESP32 Arduino cores."
#endif

namespace {

inline uint32_t rotr(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

const uint32_t SHA256_K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
};

String sha256HexBytes(const uint8_t* data, size_t len) {
    uint64_t bitLen = static_cast<uint64_t>(len) * 8;
    size_t paddedLen = len + 1;
    while ((paddedLen % 64) != 56) {
        paddedLen++;
    }
    paddedLen += 8;

    uint8_t* msg = new uint8_t[paddedLen]();
    memcpy(msg, data, len);
    msg[len] = 0x80;
    for (int i = 0; i < 8; i++) {
        msg[paddedLen - 1 - i] = static_cast<uint8_t>(bitLen >> (8 * i));
    }

    uint32_t h[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19,
    };

    for (size_t offset = 0; offset < paddedLen; offset += 64) {
        uint32_t w[64];
        for (int i = 0; i < 16; i++) {
            size_t j = offset + i * 4;
            w[i] = (static_cast<uint32_t>(msg[j]) << 24)
                | (static_cast<uint32_t>(msg[j + 1]) << 16)
                | (static_cast<uint32_t>(msg[j + 2]) << 8)
                | static_cast<uint32_t>(msg[j + 3]);
        }
        for (int i = 16; i < 64; i++) {
            uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
        uint32_t e = h[4], f = h[5], g = h[6], hh = h[7];

        for (int i = 0; i < 64; i++) {
            uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            uint32_t ch = (e & f) ^ ((~e) & g);
            uint32_t temp1 = hh + s1 + ch + SHA256_K[i] + w[i];
            uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t temp2 = s0 + maj;
            hh = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        h[0] += a;
        h[1] += b;
        h[2] += c;
        h[3] += d;
        h[4] += e;
        h[5] += f;
        h[6] += g;
        h[7] += hh;
    }

    delete[] msg;

    char out[65];
    for (int i = 0; i < 8; i++) {
        snprintf(out + i * 8, 9, "%08x", h[i]);
    }
    out[64] = '\0';
    return String(out);
}

} // namespace

LibreLinkUpTrendArrow libreLinkUpTrendArrowFromInt(int value) {
    switch (value) {
        case 1:
            return LibreLinkUpTrendArrow::DownFast;
        case 2:
            return LibreLinkUpTrendArrow::DownSlow;
        case 3:
            return LibreLinkUpTrendArrow::Stable;
        case 4:
            return LibreLinkUpTrendArrow::UpSlow;
        case 5:
            return LibreLinkUpTrendArrow::UpFast;
        default:
            return LibreLinkUpTrendArrow::Unknown;
    }
}

LibreLinkUpMeasurementColor libreLinkUpMeasurementColorFromInt(int value) {
    switch (value) {
        case 1:
            return LibreLinkUpMeasurementColor::InTarget;
        case 2:
            return LibreLinkUpMeasurementColor::High;
        case 3:
            return LibreLinkUpMeasurementColor::VeryHigh;
        case 4:
            return LibreLinkUpMeasurementColor::Low;
        default:
            return LibreLinkUpMeasurementColor::Unknown;
    }
}

const char* libreLinkUpTrendArrowName(LibreLinkUpTrendArrow trend) {
    switch (trend) {
        case LibreLinkUpTrendArrow::DownFast:
            return "down_fast";
        case LibreLinkUpTrendArrow::DownSlow:
            return "down_slow";
        case LibreLinkUpTrendArrow::Stable:
            return "stable";
        case LibreLinkUpTrendArrow::UpSlow:
            return "up_slow";
        case LibreLinkUpTrendArrow::UpFast:
            return "up_fast";
        default:
            return "unknown";
    }
}

const char* libreLinkUpTrendArrowSymbol(LibreLinkUpTrendArrow trend) {
    switch (trend) {
        case LibreLinkUpTrendArrow::DownFast:
            return "vv";
        case LibreLinkUpTrendArrow::DownSlow:
            return "v";
        case LibreLinkUpTrendArrow::Stable:
            return "->";
        case LibreLinkUpTrendArrow::UpSlow:
            return "^";
        case LibreLinkUpTrendArrow::UpFast:
            return "^^";
        default:
            return "?";
    }
}

const char* libreLinkUpMeasurementColorName(LibreLinkUpMeasurementColor color) {
    switch (color) {
        case LibreLinkUpMeasurementColor::InTarget:
            return "in_target";
        case LibreLinkUpMeasurementColor::High:
            return "high";
        case LibreLinkUpMeasurementColor::VeryHigh:
            return "very_high";
        case LibreLinkUpMeasurementColor::Low:
            return "low";
        default:
            return "unknown";
    }
}

LibreLinkUpClient::LibreLinkUpClient(
    const String& email,
    const String& password,
    const String& baseUrl,
    const String& version
) : _email(email),
    _password(password),
    _baseUrl(baseUrl),
    _version(version) {
    if (_baseUrl.endsWith("/")) {
        _baseUrl.remove(_baseUrl.length() - 1);
    }
}

bool LibreLinkUpClient::login() {
    JsonDocument body;
    body["email"] = _email;
    body["password"] = _password;

    String payload;
    serializeJson(body, payload);

    String response;
    if (!request("POST", "/llu/auth/login", payload, response)) {
        return false;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response);
    if (err) {
        _lastError = String("login JSON parse failed: ") + err.c_str();
        return false;
    }

    if (doc["data"]["redirect"].as<bool>()) {
        _lastError = String("login redirected to region: ")
            + doc["data"]["region"].as<const char*>();
        return false;
    }

    const char* token = doc["data"]["authTicket"]["token"] | nullptr;
    const char* userId = doc["data"]["user"]["id"] | nullptr;
    if (!token || !userId) {
        _lastError = "login response missing authTicket token or user id";
        return false;
    }

    _authToken = token;
    _accountId = sha256Hex(String(userId));
    return true;
}

bool LibreLinkUpClient::getLatestReading(LibreLinkUpReading& reading, uint8_t connectionIndex) {
    if (_authToken.length() == 0 && !login()) {
        return false;
    }

    String patientId;
    String patientName;
    if (!getConnection(connectionIndex, patientId, patientName)) {
        return false;
    }

    String response;
    if (!request("GET", "/llu/connections/" + patientId + "/graph", "", response)) {
        return false;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response);
    if (err) {
        _lastError = String("graph JSON parse failed: ") + err.c_str();
        return false;
    }

    JsonVariant raw = doc["data"]["connection"]["glucoseMeasurement"];
    if (raw.isNull()) {
        _lastError = "graph response missing glucoseMeasurement";
        return false;
    }

    reading.patientName = patientName;
    reading.timestamp = raw["Timestamp"].as<String>();
    reading.valueMgDl = raw["ValueInMgPerDl"] | 0;
    reading.value = raw["Value"] | 0.0;
    reading.trendArrow = raw["TrendArrow"] | 0;
    reading.measurementColor = raw["MeasurementColor"] | 0;
    reading.trend = libreLinkUpTrendArrowFromInt(reading.trendArrow);
    reading.color = libreLinkUpMeasurementColorFromInt(reading.measurementColor);
    return true;
}

const String& LibreLinkUpClient::lastError() const {
    return _lastError;
}

const String& LibreLinkUpClient::authToken() const {
    return _authToken;
}

const String& LibreLinkUpClient::accountId() const {
    return _accountId;
}

bool LibreLinkUpClient::request(
    const String& method,
    const String& path,
    const String& body,
    String& response
) {
#if defined(ESP32)
    WiFiClientSecure secureClient;
    secureClient.setInsecure();
    HTTPClient http;
#elif defined(ESP8266)
    BearSSL::WiFiClientSecure secureClient;
    secureClient.setInsecure();
    HTTPClient http;
#endif

    String url = _baseUrl + path;
    if (!http.begin(secureClient, url)) {
        _lastError = "HTTP begin failed for " + url;
        return false;
    }

    http.addHeader("product", "llu.android");
    http.addHeader("version", _version);
    // Arduino HTTP stacks do not transparently decompress gzip.
    http.addHeader("Accept-Encoding", "identity");
    http.addHeader("Cache-Control", "no-cache");
    http.addHeader("Connection", "Keep-Alive");
    http.addHeader("Content-Type", "application/json");
    http.addHeader(
        "User-Agent",
        "Mozilla/5.0 (Linux; Android 10; Pixel 3) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/88.0.4324.181 Mobile Safari/537.36"
    );
    if (_authToken.length() > 0) {
        http.addHeader("authorization", "Bearer " + _authToken);
    }
    if (_accountId.length() > 0) {
        http.addHeader("Account-Id", _accountId);
    }

    int status = 0;
    if (method == "POST") {
        status = http.POST(body);
    } else {
        status = http.GET();
    }

    response = http.getString();
    http.end();

    if (status < 200 || status >= 300) {
        _lastError = "HTTP " + String(status) + ": " + response;
        return false;
    }
    return true;
}

bool LibreLinkUpClient::getConnection(
    uint8_t index,
    String& patientId,
    String& patientName
) {
    String response;
    if (!request("GET", "/llu/connections", "", response)) {
        return false;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, response);
    if (err) {
        _lastError = String("connections JSON parse failed: ") + err.c_str();
        return false;
    }

    JsonArray connections = doc["data"].as<JsonArray>();
    if (connections.isNull() || index >= connections.size()) {
        _lastError = "no LibreLinkUp connection at index " + String(index);
        return false;
    }

    JsonVariant connection = connections[index];
    patientId = connection["patientId"].as<String>();
    String firstName = connection["firstName"].as<String>();
    String lastName = connection["lastName"].as<String>();
    patientName = firstName + (lastName.length() ? " " + lastName : "");

    if (patientId.length() == 0) {
        _lastError = "connection missing patientId";
        return false;
    }
    return true;
}

void LibreLinkUpClient::setCommonHeaders(void*) {
}

String LibreLinkUpClient::sha256Hex(const String& input) {
    return sha256HexBytes(
        reinterpret_cast<const uint8_t*>(input.c_str()),
        input.length()
    );
}
