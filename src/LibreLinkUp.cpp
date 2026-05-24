#include "LibreLinkUp.h"
#include <ArduinoJson.h>

#if defined(ESP32)
#include <esp_http_client.h>
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

#if defined(ESP32)
const char LIBRELINKUP_GTS_ROOT_R4[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIICCTCCAY6gAwIBAgINAgPlwGjvYxqccpBQUjAKBggqhkjOPQQDAzBHMQswCQYD
VQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEUMBIG
A1UEAxMLR1RTIFJvb3QgUjQwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAwMDAw
WjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2Vz
IExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjQwdjAQBgcqhkjOPQIBBgUrgQQAIgNi
AATzdHOnaItgrkO4NcWBMHtLSZ37wWHO5t5GvWvVYRg1rkDdc/eJkTBa6zzuhXyi
QHY7qca4R9gq55KRanPpsXI5nymfopjTX15YhmUPoYRlBtHci8nHc8iMai/lxKvR
HYqjQjBAMA4GA1UdDwEB/wQEAwIBhjAPBgNVHRMBAf8EBTADAQH/MB0GA1UdDgQW
BBSATNbrdP9JNqPV2Py1PsVq8JQdjDAKBggqhkjOPQQDAwNpADBmAjEA6ED/g94D
9J+uHXqnLrmvT/aDHQ4thQEd0dlq7A/Cr8deVl5c1RxYIigL9zC2L7F8AjEA8GE8
p/SgguMh1YQdc4acLa/KNJvxn7kjNuK8YAOdgLOaVsjh4rsUecrNIdSUtUlD
-----END CERTIFICATE-----
)EOF";
#endif

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

int base64UrlValue(char c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A';
    }
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 26;
    }
    if (c >= '0' && c <= '9') {
        return c - '0' + 52;
    }
    if (c == '-' || c == '+') {
        return 62;
    }
    if (c == '_' || c == '/') {
        return 63;
    }
    return -1;
}

bool decodeBase64Url(const String& input, String& output) {
    output = "";
    uint32_t buffer = 0;
    int bits = 0;

    for (size_t i = 0; i < input.length(); i++) {
        char c = input[i];
        if (c == '=') {
            break;
        }

        int value = base64UrlValue(c);
        if (value < 0) {
            return false;
        }

        buffer = (buffer << 6) | static_cast<uint32_t>(value);
        bits += 6;

        while (bits >= 8) {
            bits -= 8;
            output += static_cast<char>((buffer >> bits) & 0xff);
        }
    }

    return true;
}

String userIdFromAuthToken(const String& token) {
    if (!token) {
        return "";
    }

    String jwt = token;
    int firstDot = jwt.indexOf('.');
    int secondDot = firstDot < 0 ? -1 : jwt.indexOf('.', firstDot + 1);
    if (firstDot < 0 || secondDot < 0 || secondDot <= firstDot + 1) {
        return "";
    }

    String payload;
    if (!decodeBase64Url(jwt.substring(firstDot + 1, secondDot), payload)) {
        return "";
    }

    JsonDocument doc;
    if (deserializeJson(doc, payload)) {
        return "";
    }

    return doc["id"].as<String>();
}

bool parseLoginResponse(JsonDocument& doc, String& authToken, String& accountId, String& error) {
    bool redirect = doc["data"]["redirect"] | false;
    if (redirect) {
        String region = doc["data"]["region"] | "";
        error = "login redirected to region";
        if (region.length() > 0) {
            error += ": " + region;
        }
        return false;
    }

    int status = doc["status"] | 0;
    if (status != 0) {
        String stepType = doc["data"]["step"]["type"] | "";
        String titleKey = doc["data"]["step"]["props"]["titleKey"] | "";

        error = "login requires LibreLinkUp account action";
        if (stepType.length() > 0) {
            error += " (" + stepType + ")";
        }
        if (titleKey.length() > 0) {
            error += ": " + titleKey;
        }
        error += "; open LibreLinkUp or LibreView and complete it";
        return false;
    }

    String token = doc["data"]["authTicket"]["token"] | "";
    String userId = doc["data"]["user"]["id"] | "";

    if (userId.length() == 0 && token.length() > 0) {
        userId = userIdFromAuthToken(token.c_str());
    }

    if (token.length() == 0 || userId.length() == 0) {
        String debug;
        serializeJson(doc, debug);
        error = "login response missing authTicket token or user id"
                "; token length=" + String(token.length()) +
                "; userId length=" + String(userId.length()) +
                "; parsed JSON=" + debug;
        return false;
    }

    authToken = token;
    accountId = sha256HexBytes(
        reinterpret_cast<const uint8_t*>(userId.c_str()),
        userId.length()
    );

    return true;
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

void LibreLinkUpClient::setup(uint8_t connectionIndex, unsigned long cacheDurationMs) {
    _connectionIndex = connectionIndex;
    _cacheDurationMs = cacheDurationMs;
    _lastUpdatedAt = 0;
    _hasReading = false;
    _updating = false;
#if defined(ESP32)
    _asyncPhase = AsyncPhase::Idle;
    _asyncResponse = "";
    _asyncBody = "";
    _asyncPatientId = "";
    _asyncPatientName = "";
    if (_asyncClient) {
        esp_http_client_cleanup(_asyncClient);
        _asyncClient = nullptr;
    }
#endif
}

void LibreLinkUpClient::loop() {
#if defined(ESP32)
    if (_updating) {
        pollAsyncRequest();
        return;
    }
#else
    if (_updating) {
        return;
    }
#endif

    unsigned long now = millis();
    bool cacheExpired = !_hasReading || (now - _lastUpdatedAt >= _cacheDurationMs);
    if (!cacheExpired) {
        return;
    }

#if defined(ESP32)
    startAsyncRefresh();
#else
    updateNow();
#endif
}

bool LibreLinkUpClient::getGraphJson(JsonDocument& doc, const String& patientId) {
#if defined(ESP8266)
    BearSSL::WiFiClientSecure secureClient;
    secureClient.setInsecure();
    HTTPClient http;
#else
    WiFiClientSecure secureClient;
    secureClient.setInsecure();
    HTTPClient http;
#endif

    String url = _baseUrl + "/llu/connections/" + patientId + "/graph";

    if (!http.begin(secureClient, url)) {
        _lastError = "HTTP begin failed for " + url;
        return false;
    }

    http.setTimeout(20000);
    http.setReuse(false);

    http.addHeader("product", "llu.android");
    http.addHeader("version", _version);
    http.addHeader("Accept-Encoding", "identity");
    http.addHeader("Cache-Control", "no-cache");
    http.addHeader("Connection", "close");
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

    int status = http.GET();

    if (status < 200 || status >= 300) {
        String body = http.getString();
        _lastError = "HTTP " + String(status) + ": " + body;
        http.end();
        return false;
    }

    JsonDocument filter;
    filter["data"]["connection"]["glucoseMeasurement"]["Timestamp"] = true;
    filter["data"]["connection"]["glucoseMeasurement"]["ValueInMgPerDl"] = true;
    filter["data"]["connection"]["glucoseMeasurement"]["Value"] = true;
    filter["data"]["connection"]["glucoseMeasurement"]["TrendArrow"] = true;
    filter["data"]["connection"]["glucoseMeasurement"]["MeasurementColor"] = true;

    DeserializationError err = deserializeJson(
        doc,
        http.getStream(),
        DeserializationOption::Filter(filter)
    );

    http.end();

    if (err) {
        _lastError = String("graph JSON parse failed: ") + err.c_str();
        return false;
    }

    return true;
}

bool LibreLinkUpClient::updateNow() {
    if (_updating) {
        return false;
    }

    _updating = true;
    LibreLinkUpReading latest;
    bool ok = getLatestReading(latest, _connectionIndex);
    _updating = false;

    if (!ok) {
        if (_errorCallback) {
            _errorCallback(_lastError);
        }
        return false;
    }

    _reading = latest;
    _hasReading = true;
    _lastUpdatedAt = millis();

    if (_updateCallback) {
        _updateCallback(_reading);
    }
    return true;
}

void LibreLinkUpClient::onUpdate(LibreLinkUpUpdateCallback callback) {
    _updateCallback = callback;
}

void LibreLinkUpClient::onError(LibreLinkUpErrorCallback callback) {
    _errorCallback = callback;
}

#if defined(ESP32)
bool LibreLinkUpClient::startAsyncRefresh() {
    if (_updating) {
        return false;
    }

    _updating = true;
    _asyncPatientId = "";
    _asyncPatientName = "";

    if (_authToken.length() == 0) {
        JsonDocument body;
        body["email"] = _email;
        body["password"] = _password;
        String payload;
        serializeJson(body, payload);
        return startAsyncRequest("POST", "/llu/auth/login", payload, AsyncPhase::Login);
    }

    return startAsyncRequest("GET", "/llu/connections", "", AsyncPhase::Connections);
}

bool LibreLinkUpClient::startAsyncRequest(
    const String& method,
    const String& path,
    const String& body,
    AsyncPhase phase
) {
    if (_asyncClient) {
        esp_http_client_cleanup(_asyncClient);
        _asyncClient = nullptr;
    }

    _asyncResponse = "";
    _asyncBody = body;
    _asyncPhase = phase;

    String url = _baseUrl + path;
    esp_http_client_config_t config = {};
    config.url = url.c_str();
    config.event_handler = LibreLinkUpClient::handleAsyncEvent;
    config.user_data = this;
    config.is_async = true;
    config.timeout_ms = 15000;
    config.cert_pem = LIBRELINKUP_GTS_ROOT_R4;

    // Important: LibreLinkUp may return headers larger than ESP-IDF's default buffer.
    config.buffer_size = 4096;
    config.buffer_size_tx = 4096;

    _asyncClient = esp_http_client_init(&config);
    if (!_asyncClient) {
        failAsync("ESP32 async HTTP init failed for " + url);
        return false;
    }

    esp_http_client_set_method(
        _asyncClient,
        method == "POST" ? HTTP_METHOD_POST : HTTP_METHOD_GET
    );
    esp_http_client_set_header(_asyncClient, "product", "llu.android");
    esp_http_client_set_header(_asyncClient, "version", _version.c_str());
    esp_http_client_set_header(_asyncClient, "Accept-Encoding", "identity");
    esp_http_client_set_header(_asyncClient, "Cache-Control", "no-cache");
    esp_http_client_set_header(_asyncClient, "Connection", "Keep-Alive");
    esp_http_client_set_header(_asyncClient, "Content-Type", "application/json");
    esp_http_client_set_header(
        _asyncClient,
        "User-Agent",
        "Mozilla/5.0 (Linux; Android 10; Pixel 3) AppleWebKit/537.36 "
        "(KHTML, like Gecko) Chrome/88.0.4324.181 Mobile Safari/537.36"
    );
    if (_authToken.length() > 0) {
        String authorization = "Bearer " + _authToken;
        esp_http_client_set_header(_asyncClient, "authorization", authorization.c_str());
    }
    if (_accountId.length() > 0) {
        esp_http_client_set_header(_asyncClient, "Account-Id", _accountId.c_str());
    }
    if (method == "POST") {
        esp_http_client_set_post_field(_asyncClient, _asyncBody.c_str(), _asyncBody.length());
    }

    pollAsyncRequest();
    return true;
}

void LibreLinkUpClient::pollAsyncRequest() {
    if (!_asyncClient) {
        return;
    }

    esp_err_t err = esp_http_client_perform(_asyncClient);
    if (err == ESP_ERR_HTTP_EAGAIN) {
        return;
    }
    if (err != ESP_OK) {
        failAsync("ESP32 async HTTP failed: " + String(esp_err_to_name(err)));
        return;
    }

    finishAsyncResponse();
}

bool LibreLinkUpClient::finishAsyncResponse() {
    int status = esp_http_client_get_status_code(_asyncClient);
    esp_http_client_cleanup(_asyncClient);
    _asyncClient = nullptr;

    if (status < 200 || status >= 300) {
        const char* phaseName = "idle";
        switch (_asyncPhase) {
            case AsyncPhase::Login:
                phaseName = "login";
                break;
            case AsyncPhase::Connections:
                phaseName = "connections";
                break;
            case AsyncPhase::Graph:
                phaseName = "graph";
                break;
            default:
                break;
        }
        failAsync(String("HTTP ") + status + " during " + phaseName + ": " + _asyncResponse);
        return false;
    }

    switch (_asyncPhase) {
        case AsyncPhase::Login:
            return handleAsyncLogin();
        case AsyncPhase::Connections:
            return handleAsyncConnections();
        case AsyncPhase::Graph:
            return handleAsyncGraph();
        default:
            failAsync("unexpected ESP32 async phase");
            return false;
    }
}

bool LibreLinkUpClient::handleAsyncLogin() {
    JsonDocument filter;
    filter["status"] = true;
    filter["data"]["redirect"] = true;
    filter["data"]["region"] = true;
    filter["data"]["user"]["id"] = true;
    filter["data"]["authTicket"]["token"] = true;
    filter["data"]["step"]["type"] = true;
    filter["data"]["step"]["props"]["titleKey"] = true;

    JsonDocument doc;
    DeserializationError err = deserializeJson(
        doc,
        _asyncResponse,
        DeserializationOption::Filter(filter)
    );

    if (err) {
        failAsync(String("login JSON parse failed: ") + err.c_str());
        return false;
    }

    String authToken;
    String accountId;
    String error;
    if (!parseLoginResponse(doc, authToken, accountId, error)) {
        failAsync(error);
        return false;
    }

    _authToken = authToken;
    _accountId = accountId;
    return startAsyncRequest("GET", "/llu/connections", "", AsyncPhase::Connections);
}

bool LibreLinkUpClient::handleAsyncConnections() {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _asyncResponse);
    if (err) {
        failAsync(String("connections JSON parse failed: ") + err.c_str());
        return false;
    }

    JsonArray connections = doc["data"].as<JsonArray>();
    if (connections.isNull() || _connectionIndex >= connections.size()) {
        failAsync("no LibreLinkUp connection at index " + String(_connectionIndex));
        return false;
    }

    JsonVariant connection = connections[_connectionIndex];
    _asyncPatientId = connection["patientId"].as<String>();
    String firstName = connection["firstName"].as<String>();
    String lastName = connection["lastName"].as<String>();
    _asyncPatientName = firstName + (lastName.length() ? " " + lastName : "");

    if (_asyncPatientId.length() == 0) {
        failAsync("connection missing patientId");
        return false;
    }

    return startAsyncRequest(
        "GET",
        "/llu/connections/" + _asyncPatientId + "/graph",
        "",
        AsyncPhase::Graph
    );
}

bool LibreLinkUpClient::handleAsyncGraph() {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _asyncResponse);
    if (err) {
        failAsync(String("graph JSON parse failed: ") + err.c_str());
        return false;
    }

    JsonVariant raw = doc["data"]["connection"]["glucoseMeasurement"];
    if (raw.isNull()) {
        failAsync("graph response missing glucoseMeasurement");
        return false;
    }

    _reading.patientName = _asyncPatientName;
    _reading.timestamp = raw["Timestamp"].as<String>();
    _reading.valueMgDl = raw["ValueInMgPerDl"] | 0;
    _reading.value = raw["Value"] | 0.0;
    _reading.trendArrow = raw["TrendArrow"] | 0;
    _reading.measurementColor = raw["MeasurementColor"] | 0;
    _reading.trend = libreLinkUpTrendArrowFromInt(_reading.trendArrow);
    _reading.color = libreLinkUpMeasurementColorFromInt(_reading.measurementColor);
    _hasReading = true;
    _lastUpdatedAt = millis();
    _asyncPhase = AsyncPhase::Idle;
    _updating = false;

    if (_updateCallback) {
        _updateCallback(_reading);
    }
    return true;
}

void LibreLinkUpClient::failAsync(const String& error) {
    _lastError = error;
    _asyncPhase = AsyncPhase::Idle;
    _updating = false;
    if (_asyncClient) {
        esp_http_client_cleanup(_asyncClient);
        _asyncClient = nullptr;
    }
    if (_errorCallback) {
        _errorCallback(_lastError);
    }
}

esp_err_t LibreLinkUpClient::handleAsyncEvent(esp_http_client_event_t* event) {
    LibreLinkUpClient* self = static_cast<LibreLinkUpClient*>(event->user_data);
    if (!self) {
        return ESP_OK;
    }

    if (event->event_id == HTTP_EVENT_ON_DATA && event->data && event->data_len > 0) {
        const char* data = static_cast<const char*>(event->data);
        for (int i = 0; i < event->data_len; i++) {
            self->_asyncResponse += data[i];
        }
    }
    return ESP_OK;
}
#endif

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

    JsonDocument filter;
    filter["status"] = true;
    filter["data"]["redirect"] = true;
    filter["data"]["region"] = true;
    filter["data"]["user"]["id"] = true;
    filter["data"]["authTicket"]["token"] = true;
    filter["data"]["step"]["type"] = true;
    filter["data"]["step"]["props"]["titleKey"] = true;

    JsonDocument doc;
    DeserializationError err = deserializeJson(
        doc,
        response,
        DeserializationOption::Filter(filter)
    );

    if (err) {
        _lastError = String("login JSON parse failed: ") + err.c_str();
        return false;
    }

    String authToken;
    String accountId;
    String error;
    if (!parseLoginResponse(doc, authToken, accountId, error)) {
        _lastError = error;
        return false;
    }

    _authToken = authToken;
    _accountId = accountId;
    return true;
}

bool LibreLinkUpClient::getLatestReading(
    LibreLinkUpReading& reading,
    uint8_t connectionIndex
) {
    if (_authToken.length() == 0 && !login()) {
        return false;
    }

    String patientId;
    String patientName;

    if (!getConnection(connectionIndex, patientId, patientName)) {
        return false;
    }

    JsonDocument doc;

    #if defined(ESP8266)
        JsonDocument filter;
        filter["data"]["connection"]["glucoseMeasurement"]["Timestamp"] = true;
        filter["data"]["connection"]["glucoseMeasurement"]["ValueInMgPerDl"] = true;
        filter["data"]["connection"]["glucoseMeasurement"]["Value"] = true;
        filter["data"]["connection"]["glucoseMeasurement"]["TrendArrow"] = true;
        filter["data"]["connection"]["glucoseMeasurement"]["MeasurementColor"] = true;


        if (!requestJson(
            "GET",
            "/llu/connections/" + patientId + "/graph",
            "",
            doc,
            &filter
        )) {
            return false;
        }
    #else
        if (!requestJson(
            "GET",
            "/llu/connections/" + patientId + "/graph",
            "",
            doc
        ))
        {
            return false;
        }
    #endif

    JsonVariant raw = doc["data"]["connection"]["glucoseMeasurement"];

    if (raw.isNull()) {
        String debug;
        serializeJson(doc, debug);
        _lastError = "graph response missing glucoseMeasurement; parsed=" + debug;
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

bool LibreLinkUpClient::hasReading() const {
    return _hasReading;
}

bool LibreLinkUpClient::isUpdating() const {
    return _updating;
}

unsigned long LibreLinkUpClient::lastUpdatedAt() const {
    return _lastUpdatedAt;
}

const LibreLinkUpReading& LibreLinkUpClient::reading() const {
    return _reading;
}

const String& LibreLinkUpClient::patientName() const {
    return _reading.patientName;
}

const String& LibreLinkUpClient::timestamp() const {
    return _reading.timestamp;
}

int LibreLinkUpClient::valueMgDl() const {
    return _reading.valueMgDl;
}

float LibreLinkUpClient::value() const {
    return _reading.value;
}

LibreLinkUpTrendArrow LibreLinkUpClient::trend() const {
    return _reading.trend;
}

LibreLinkUpMeasurementColor LibreLinkUpClient::color() const {
    return _reading.color;
}

const char* LibreLinkUpClient::trendName() const {
    return libreLinkUpTrendArrowName(_reading.trend);
}

const char* LibreLinkUpClient::trendSymbol() const {
    return libreLinkUpTrendArrowSymbol(_reading.trend);
}

const char* LibreLinkUpClient::colorName() const {
    return libreLinkUpMeasurementColorName(_reading.color);
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

bool LibreLinkUpClient::requestJson(
    const String& method,
    const String& path,
    const String& body,
    JsonDocument& doc,
    JsonDocument* filter
) {
#if defined(ESP32)
    WiFiClientSecure secureClient;
    secureClient.setInsecure();
#elif defined(ESP8266)
    BearSSL::WiFiClientSecure secureClient;
    secureClient.setInsecure();
    secureClient.setBufferSizes(2048, 1024);
#endif

    HTTPClient http;
    String url = _baseUrl + path;

    if (!http.begin(secureClient, url)) {
        _lastError = "HTTP begin failed for " + url;
        return false;
    }

    http.setTimeout(25000);
    http.setReuse(false);

#if defined(ESP8266)
    http.useHTTP10(true);
#endif

    http.addHeader("product", "llu.android");
    http.addHeader("version", _version);
    http.addHeader("Accept-Encoding", "identity");
    http.addHeader("Cache-Control", "no-cache");
    http.addHeader("Connection", "close");
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

    int status = method == "POST" ? http.POST(body) : http.GET();

    if (status < 200 || status >= 300) {
        String errorBody = http.getString();
        _lastError = "HTTP " + String(status) + " for " + path + ": " + errorBody;
        http.end();
        return false;
    }

    DeserializationError err;

#if defined(ESP32)
    String response = http.getString();
    if (filter) {
        err = deserializeJson(doc, response, DeserializationOption::Filter(*filter));
    } else {
        err = deserializeJson(doc, response);
    }
#else
    if (filter) {
        err = deserializeJson(
            doc,
            http.getStream(),
            DeserializationOption::Filter(*filter)
        );
    } else {
        err = deserializeJson(doc, http.getStream());
    }
#endif

    http.end();

    if (err) {
        _lastError = String(path) + " JSON parse failed: " + err.c_str();
        return false;
    }

    return true;
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
    secureClient.setBufferSizes(2048, 1024);
    HTTPClient http;
#endif

    String url = _baseUrl + path;
    if (!http.begin(secureClient, url)) {
        _lastError = "HTTP begin failed for " + url;
        return false;
    }

    http.setTimeout(15000);
    http.setReuse(false);

    http.addHeader("product", "llu.android");
    http.addHeader("version", _version);
    http.addHeader("Accept-Encoding", "identity");
    http.addHeader("Cache-Control", "no-cache");
    http.addHeader("Connection", "close");
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
    JsonDocument filter;
    filter["data"][0]["patientId"] = true;
    filter["data"][0]["firstName"] = true;
    filter["data"][0]["lastName"] = true;

    JsonDocument doc;

    if (!requestJson("GET", "/llu/connections", "", doc, &filter)) {
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
