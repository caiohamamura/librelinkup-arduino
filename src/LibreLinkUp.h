#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#if defined(ESP32)
#include <esp_http_client.h>
#endif

enum class LibreLinkUpTrendArrow : uint8_t {
    Unknown = 0,
    DownFast = 1,
    DownSlow = 2,
    Stable = 3,
    UpSlow = 4,
    UpFast = 5,
};

enum class LibreLinkUpMeasurementColor : uint8_t {
    Unknown = 0,
    InTarget = 1,
    High = 2,
    VeryHigh = 3,
    Low = 4,
};

struct LibreLinkUpReading {
    String patientName;
    String timestamp;
    int valueMgDl = 0;
    float value = 0;
    int trendArrow = 0;
    int measurementColor = 0;
    LibreLinkUpTrendArrow trend = LibreLinkUpTrendArrow::Unknown;
    LibreLinkUpMeasurementColor color = LibreLinkUpMeasurementColor::Unknown;
};

typedef void (*LibreLinkUpUpdateCallback)(const LibreLinkUpReading& reading);
typedef void (*LibreLinkUpErrorCallback)(const String& error);

LibreLinkUpTrendArrow libreLinkUpTrendArrowFromInt(int value);
LibreLinkUpMeasurementColor libreLinkUpMeasurementColorFromInt(int value);
const char* libreLinkUpTrendArrowName(LibreLinkUpTrendArrow trend);
const char* libreLinkUpTrendArrowSymbol(LibreLinkUpTrendArrow trend);
const char* libreLinkUpMeasurementColorName(LibreLinkUpMeasurementColor color);
String userIdFromAuthToken(const String& token);

class LibreLinkUpClient {
public:
    LibreLinkUpClient(
        const String& email,
        const String& password,
        const String& baseUrl = "https://api.libreview.io",
        const String& version = "4.16.0"
    );

    void setup(uint8_t connectionIndex = 0, unsigned long cacheDurationMs = 60000);
    void loop();
    bool updateNow();

    void onUpdate(LibreLinkUpUpdateCallback callback);
    void onError(LibreLinkUpErrorCallback callback);

    bool login();
    bool getLatestReading(LibreLinkUpReading& reading, uint8_t connectionIndex = 0);

    bool hasReading() const;
    bool isUpdating() const;
    unsigned long lastUpdatedAt() const;
    const LibreLinkUpReading& reading() const;
    const String& patientName() const;
    const String& timestamp() const;
    int valueMgDl() const;
    float value() const;
    LibreLinkUpTrendArrow trend() const;
    LibreLinkUpMeasurementColor color() const;
    const char* trendName() const;
    const char* trendSymbol() const;
    const char* colorName() const;

    const String& lastError() const;
    const String& authToken() const;
    const String& accountId() const;

private:
#if defined(ESP32)
    enum class AsyncPhase : uint8_t {
        Idle,
        Login,
        Connections,
        Graph,
    };
#endif

    String _email;
    String _password;
    String _baseUrl;
    String _version;
    String _authToken;
    String _accountId;
    String _lastError;
    LibreLinkUpReading _reading;
    bool _hasReading = false;
    bool _updating = false;
    uint8_t _connectionIndex = 0;
    unsigned long _cacheDurationMs = 60000;
    unsigned long _lastUpdatedAt = 0;
    LibreLinkUpUpdateCallback _updateCallback = nullptr;
    LibreLinkUpErrorCallback _errorCallback = nullptr;
#if defined(ESP32)
    esp_http_client_handle_t _asyncClient = nullptr;
    AsyncPhase _asyncPhase = AsyncPhase::Idle;
    String _asyncResponse;
    String _asyncBody;
    String _asyncPatientId;
    String _asyncPatientName;
#endif

    bool request(const String& method, const String& path, const String& body, String& response);
    bool getConnection(uint8_t index, String& patientId, String& patientName);
    void setCommonHeaders(void* httpClient);
    String sha256Hex(const String& input);
    bool getGraphJson(JsonDocument& doc, const String& patientId);
    bool requestJson(
        const String& method,
        const String& path,
        const String& body,
        JsonDocument& doc,
        JsonDocument* filter = nullptr
    );
#if defined(ESP32)
    bool startAsyncRefresh();
    bool startAsyncRequest(const String& method, const String& path, const String& body, AsyncPhase phase);
    void pollAsyncRequest();
    bool finishAsyncResponse();
    bool handleAsyncLogin();
    bool handleAsyncConnections();
    bool handleAsyncGraph();
    void failAsync(const String& error);
    static esp_err_t handleAsyncEvent(esp_http_client_event_t* event);
#endif
};
