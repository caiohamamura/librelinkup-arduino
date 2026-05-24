#pragma once

#include <Arduino.h>

struct LibreLinkUpReading {
    String patientName;
    String timestamp;
    int valueMgDl = 0;
    float value = 0;
    int trendArrow = 0;
    int measurementColor = 0;
};

class LibreLinkUpClient {
public:
    LibreLinkUpClient(
        const String& email,
        const String& password,
        const String& baseUrl = "https://api.libreview.io",
        const String& version = "4.16.0"
    );

    bool login();
    bool getLatestReading(LibreLinkUpReading& reading, uint8_t connectionIndex = 0);

    const String& lastError() const;
    const String& authToken() const;
    const String& accountId() const;

private:
    String _email;
    String _password;
    String _baseUrl;
    String _version;
    String _authToken;
    String _accountId;
    String _lastError;

    bool request(const String& method, const String& path, const String& body, String& response);
    bool getConnection(uint8_t index, String& patientId, String& patientName);
    void setCommonHeaders(void* httpClient);
    String sha256Hex(const String& input);
};
