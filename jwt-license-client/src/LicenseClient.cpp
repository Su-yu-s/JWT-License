#include "LicenseClient.h"
#include "Base64.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <iostream>
#include <algorithm>

#ifdef _WIN32
#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <winhttp.h>
#include <wincrypt.h>
#include <bcrypt.h>
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "ncrypt.lib")
#pragma comment(lib, "crypt32.lib")
#endif

namespace jwt_client {
namespace {

// Parse PEM public key to DER bytes
bool pemToDerPublicKey(const std::string& pem, std::vector<unsigned char>& derOut) {
    size_t begin = pem.find("-----BEGIN PUBLIC KEY-----");
    if (begin == std::string::npos) return false;
    size_t end = pem.find("-----END PUBLIC KEY-----", begin);
    if (end == std::string::npos) return false;

    // 提取 BEGIN 和 END 之间的 base64 内容
    size_t b64Start = begin + 26;  // 跳过 "-----BEGIN PUBLIC KEY-----"
    size_t b64Len = end - b64Start;
    std::string b64 = pem.substr(b64Start, b64Len);

    // 去除所有空白字符
    b64.erase(std::remove_if(b64.begin(), b64.end(),
        [](unsigned char c) { return c==' '||c=='\t'||c=='\r'||c=='\n'; }), b64.end());

    if (b64.empty()) return false;

    std::string decoded = jwt_client::base64Decode(b64);
    derOut.assign(decoded.begin(), decoded.end());
    return !derOut.empty();
}

// Decode base64url to bytes
std::vector<unsigned char> b64url_decode(const std::string& s) {
    std::string restored = s;
    for (auto& c : restored) {
        if (c == '-') c = '+';
        else if (c == '_') c = '/';
    }
    while (restored.size() % 4) restored += '=';
    std::string decoded = jwt_client::base64Decode(restored);
    return std::vector<unsigned char>(decoded.begin(), decoded.end());
}

// QJson helpers (replacing nlohmann::json)
static QJsonDocument safeParse(const std::string& raw) {
    return QJsonDocument::fromJson(QByteArray::fromRawData(raw.data(), (int)raw.size()), {});
}

static QString jval(const QJsonObject& obj, const QString& key, const QString& def = {}) {
    auto it = obj.find(key);
    return (it != obj.end() && !it->isNull()) ? it->toString(def) : def;
}

static QString jval_audience(const QJsonObject& obj) {
    auto it = obj.find("aud");
    if (it == obj.end() || it->isNull()) return {};
    if (it->isString()) return it->toString();
    if (it->isArray()) {
        QJsonArray arr = it->toArray();
        return arr.isEmpty() ? QString{} : arr.first().toString();
    }
    return {};
}

static long long jval_ll(const QJsonObject& obj, const QString& key, long long def = 0) {
    auto it = obj.find(key);
    if (it == obj.end() || it->isNull()) return def;
    bool ok = false;
    long long val = it->toVariant().toLongLong(&ok);
    return ok ? val : def;
}

static int jval_int(const QJsonObject& obj, const QString& key, int def = 0) {
    auto it = obj.find(key);
    if (it == obj.end() || it->isNull()) return def;
    bool ok = false;
    int val = it->toVariant().toInt(&ok);
    return ok ? val : def;
}

static bool jval_bool(const QJsonObject& obj, const QString& key, bool def = false) {
    auto it = obj.find(key);
    return (it != obj.end() && !it->isNull()) ? it->toBool(def) : def;
}

} // anonymous namespace

// ==================== Constructor ====================

LicenseClient::LicenseClient(const std::string& serverUrl, ProgressCallback log)
    : baseUrl_(serverUrl), log_(std::move(log)) {
    while (!baseUrl_.empty() && baseUrl_.back() == '/') baseUrl_.pop_back();
}

void LicenseClient::setExpectedProductId(const std::string& productId) {
    expectedProductId_ = productId;
}

// ==================== HTTP Layer (WinHTTP UNICODE) ====================

static std::wstring strToWstr(const std::string& s) {
    if (s.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (len <= 0) return L"";
    std::wstring w(len, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &w[0], len);
    w.pop_back();
    return w;
}

std::string LicenseClient::postRequest(const std::string& path, const std::string& body) {
    std::string fullUrl = baseUrl_ + "/" + path;

    bool isHttps = fullUrl.find("https://") == 0;
    if (isHttps) fullUrl = fullUrl.substr(8);
    else if (fullUrl.find("http://") == 0) fullUrl = fullUrl.substr(7);

    size_t slashPos = fullUrl.find('/');
    std::string hostPort = fullUrl.substr(0, slashPos);
    std::string requestPath = slashPos != std::string::npos ? fullUrl.substr(slashPos) : "/";

    size_t colonPos = hostPort.find(':');
    std::string hostStr;
    INTERNET_PORT port = isHttps ? 443 : 80;
    if (colonPos != std::string::npos) {
        hostStr = hostPort.substr(0, colonPos);
        port = (INTERNET_PORT)std::stoi(hostPort.substr(colonPos + 1));
    } else {
        hostStr = hostPort;
    }

    HINTERNET hSession = WinHttpOpen(strToWstr("JwtLicenseClient/1.0").c_str(),
                                      0,
                                      WINHTTP_NO_PROXY_NAME,
                                      WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) { lastError_ = "WinHttpOpen failed"; return ""; }

    HINTERNET hConnect = WinHttpConnect(hSession, strToWstr(hostStr).c_str(), port, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); lastError_ = "WinHttpConnect failed"; return ""; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect,
                                             strToWstr("POST").c_str(),
                                             strToWstr(requestPath).c_str(),
                                             nullptr,
                                             WINHTTP_NO_REFERER,
                                             WINHTTP_DEFAULT_ACCEPT_TYPES,
                                             isHttps ? WINHTTP_FLAG_SECURE : 0);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        lastError_ = "WinHttpOpenRequest failed";
        return "";
    }

    std::string hdrs = "Content-Type: application/json\r\nAccept: application/json\r\n";
    std::wstring wHdrs = strToWstr(hdrs);

	    if (!WinHttpSendRequest(hRequest, wHdrs.c_str(), -1L,
	                            const_cast<void*>(static_cast<const void*>(body.c_str())),
	                            (DWORD)body.size(), (DWORD)body.size(), 0)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        lastError_ = "WinHttpSendRequest failed";
        return "";
    }

    if (!WinHttpReceiveResponse(hRequest, nullptr)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        lastError_ = "WinHttpReceiveResponse failed";
        return "";
    }

    DWORD statusCode = 0;
    DWORD statusLen = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusLen, WINHTTP_NO_HEADER_INDEX);
    lastStatusCode_ = (int)statusCode;

    DWORD bytesRead = 0;
    std::string result;
    BYTE buf[8192];
    do {
        bytesRead = 0;
        if (!WinHttpReadData(hRequest, buf, sizeof(buf), &bytesRead)) break;
        result.append(reinterpret_cast<char*>(buf), bytesRead);
    } while (bytesRead > 0);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    if (log_) log_("HTTP " + std::to_string(lastStatusCode_) + " POST " + path);
    return result;
}

// Helper for POST with query params (heartbeat, online-check, deactivate)
static std::string winHttpPostWithQuery(const std::string& baseUrl,
                                         const std::string& path,
                                         const std::string& query,
                                         int& statusCode,
                                         std::string& lastError) {
    std::string fullUrl = baseUrl + "/" + path;
    bool isHttps = fullUrl.find("https://") == 0;
    if (isHttps) fullUrl = fullUrl.substr(8);
    else if (fullUrl.find("http://") == 0) fullUrl = fullUrl.substr(7);

    size_t slashPos = fullUrl.find('/');
    std::string hostPort = fullUrl.substr(0, slashPos);
    std::string requestPath = slashPos != std::string::npos ? fullUrl.substr(slashPos) : "/";
    if (!query.empty()) requestPath += "?" + query;

    size_t colonPos = hostPort.find(':');
    std::string hostStr;
    INTERNET_PORT port = isHttps ? 443 : 80;
    if (colonPos != std::string::npos) {
        hostStr = hostPort.substr(0, colonPos);
        port = (INTERNET_PORT)std::stoi(hostPort.substr(colonPos + 1));
    } else {
        hostStr = hostPort;
    }

    HINTERNET hSession = WinHttpOpen(strToWstr("JwtLicenseClient/1.0").c_str(),
                                      0, 0, 0, 0);
    if (!hSession) { lastError = "WinHttpOpen failed"; return ""; }

    HINTERNET hConnect = WinHttpConnect(hSession, strToWstr(hostStr).c_str(), port, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); lastError = "WinHttpConnect failed"; return ""; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect,
                                             strToWstr("POST").c_str(),
                                             strToWstr(requestPath).c_str(),
                                             nullptr, WINHTTP_NO_REFERER,
                                             WINHTTP_DEFAULT_ACCEPT_TYPES,
                                             isHttps ? WINHTTP_FLAG_SECURE : 0);
    if (!hRequest) {
        WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        lastError = "WinHttpOpenRequest failed";
        return "";
    }

    std::string hdrs = "Accept: application/json\r\n";
    std::wstring wHdrs = strToWstr(hdrs);
	    if (!WinHttpSendRequest(hRequest, wHdrs.c_str(), -1L,
	                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        DWORD err = GetLastError();
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        lastError = "WinHttpSendRequest failed (error=" + std::to_string(err) + ")";
        return "";
    }
    if (!WinHttpReceiveResponse(hRequest, nullptr)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        lastError = "WinHttpReceiveResponse failed";
        return "";
    }

    DWORD sc = 0;
    DWORD slen = sizeof(sc);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &sc, &slen, WINHTTP_NO_HEADER_INDEX);
    statusCode = (int)sc;

    DWORD bytesRead = 0;
    std::string result;
    BYTE buf[8192];
    do {
        bytesRead = 0;
        if (!WinHttpReadData(hRequest, buf, sizeof(buf), &bytesRead)) break;
        result.append(reinterpret_cast<char*>(buf), bytesRead);
    } while (bytesRead > 0);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);
    return result;
}

std::string LicenseClient::getRequest(const std::string& path) {
    std::string fullUrl = baseUrl_ + "/" + path;
    bool isHttps = fullUrl.find("https://") == 0;
    if (isHttps) fullUrl = fullUrl.substr(8);
    else if (fullUrl.find("http://") == 0) fullUrl = fullUrl.substr(7);

    size_t slashPos = fullUrl.find('/');
    std::string hostPort = fullUrl.substr(0, slashPos);
    std::string requestPath = slashPos != std::string::npos ? fullUrl.substr(slashPos) : "/";

    size_t colonPos = hostPort.find(':');
    std::string hostStr;
    INTERNET_PORT port = isHttps ? 443 : 80;
    if (colonPos != std::string::npos) {
        hostStr = hostPort.substr(0, colonPos);
        port = (INTERNET_PORT)std::stoi(hostPort.substr(colonPos + 1));
    } else {
        hostStr = hostPort;
    }

    HINTERNET hSession = WinHttpOpen(strToWstr("JwtLicenseClient/1.0").c_str(),
                                      0, 0, 0, 0);
    if (!hSession) { lastError_ = "WinHttpOpen failed"; return ""; }

    HINTERNET hConnect = WinHttpConnect(hSession, strToWstr(hostStr).c_str(), port, 0);
    if (!hConnect) { WinHttpCloseHandle(hSession); lastError_ = "WinHttpConnect failed"; return ""; }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect,
                                             strToWstr("GET").c_str(),
                                             strToWstr(requestPath).c_str(),
                                             nullptr, WINHTTP_NO_REFERER,
                                             WINHTTP_DEFAULT_ACCEPT_TYPES,
                                             isHttps ? WINHTTP_FLAG_SECURE : 0);
    if (!hRequest) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); lastError_ = "WinHttpOpenRequest failed"; return ""; }

    if (!WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        lastError_ = "WinHttpSendRequest failed";
        return "";
    }
    if (!WinHttpReceiveResponse(hRequest, nullptr)) {
        WinHttpCloseHandle(hRequest); WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession);
        lastError_ = "WinHttpReceiveResponse failed";
        return "";
    }

    DWORD sc = 0;
    DWORD slen = sizeof(sc);
    WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &sc, &slen, WINHTTP_NO_HEADER_INDEX);
    lastStatusCode_ = (int)sc;

    DWORD bytesRead = 0;
    std::string result;
    BYTE buf[8192];
    do {
        bytesRead = 0;
        if (!WinHttpReadData(hRequest, buf, sizeof(buf), &bytesRead)) break;
        result.append(reinterpret_cast<char*>(buf), bytesRead);
    } while (bytesRead > 0);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    if (log_) log_("HTTP " + std::to_string(lastStatusCode_) + " GET " + path);
    return result;
}

// ==================== Core SDK Methods ====================

ActivateResponse LicenseClient::activate(const std::string& keyCode,
                                          const std::string& machineHash,
                                          const std::string& sdkVersion) {
    ActivateResponse resp{};

    QJsonObject j;
    j["keyCode"] = QString::fromStdString(keyCode);
    j["machineHash"] = QString::fromStdString(machineHash);
    j["sdkVersion"] = QString::fromStdString(sdkVersion);

    QJsonDocument doc(j);
    std::string raw = postRequest("api/sdk/activate", std::string(doc.toJson(QJsonDocument::Compact).data()));
    if (raw.empty()) { resp.errorMessage = lastError_; resp.errorCode = "SERVER_UNREACHABLE"; return resp; }

    if (lastStatusCode_ >= 400) {
        // 尝试从响应体中提取错误信息
        auto jsonDoc = safeParse(raw);
        QJsonObject root = jsonDoc.object();
        QString serverMsg = jval(root, "message");
        resp.errorMessage = serverMsg.isEmpty()
            ? "服务器返回错误 (HTTP " + std::to_string(lastStatusCode_) + ")"
            : serverMsg.toStdString();
        resp.errorCode = "SERVER_ERROR";
        return resp;
    }

    auto jsonDoc = safeParse(raw);
    QJsonObject data = jsonDoc.object().value("data").toObject();
    if (data.isEmpty()) { resp.errorMessage = "服务器响应格式异常"; resp.errorCode = "SERVER_ERROR"; return resp; }

    resp.success = jval_bool(data, "success");
    resp.token = jval(data, "token").toStdString();
    resp.issuedAt = jval(data, "issuedAt").toStdString();
    resp.expiresAt = jval(data, "expiresAt").toStdString();
    resp.sessionId = jval_ll(data, "sessionId");
    resp.capacity = jval_int(data, "capacity");
    resp.activeSeats = jval_int(data, "activeSeats");
    resp.publicKeyPem = jval(data, "publicKeyPem").toStdString();
    resp.issuer = jval(data, "issuer").toStdString();
    resp.errorCode = jval(data, "errorCode").toStdString();
    resp.errorMessage = jval(data, "message").toStdString();

    auto occ = data.value("occupied");
    if (!occ.isNull() && occ.isObject()) {
        resp.occupied.sessionId = jval_ll(occ.toObject(), "sessionId");
        resp.occupied.machineHashPrefix = jval(occ.toObject(), "machineHashPrefix").toStdString();
        resp.occupied.lastSeenAt = jval(occ.toObject(), "lastSeenAt").toStdString();
    }

    return resp;
}

HeartbeatResponse LicenseClient::heartbeat(const std::string& keyCode) {
    HeartbeatResponse resp{};
    int sc = 0;
    std::string raw = winHttpPostWithQuery(baseUrl_, "api/sdk/heartbeat", "keyCode=" + keyCode, sc, lastError_);

    if (raw.empty()) { resp.errorMessage = lastError_; return resp; }
    lastStatusCode_ = sc;

    if (sc >= 400) {
        auto jsonDoc = safeParse(raw);
        QJsonObject root = jsonDoc.object();
        QString serverMsg = jval(root, "message");
        resp.errorMessage = serverMsg.isEmpty()
            ? "服务器返回错误 (HTTP " + std::to_string(sc) + ")"
            : serverMsg.toStdString();
        return resp;
    }

    auto jsonDoc = safeParse(raw);
    QJsonObject data = jsonDoc.object().value("data").toObject();
    if (data.isEmpty()) { resp.errorMessage = "服务器响应格式异常"; return resp; }

    resp.success = jval_bool(data, "success");
    resp.token = jval(data, "token").toStdString();
    resp.expiresAt = jval(data, "expiresAt").toStdString();
    resp.sessionId = jval_ll(data, "sessionId");
    resp.activeSeats = jval_int(data, "activeSeats");
    resp.publicKeyPem = jval(data, "publicKeyPem").toStdString();
    resp.issuer = jval(data, "issuer").toStdString();

    return resp;
}

OnlineCheckResponse LicenseClient::onlineCheck(const std::string& keyCode) {
    OnlineCheckResponse resp{};
    int sc = 0;
    std::string raw = winHttpPostWithQuery(baseUrl_, "api/sdk/online-check", "keyCode=" + keyCode, sc, lastError_);

    if (raw.empty()) { resp.errorMessage = lastError_; return resp; }
    lastStatusCode_ = sc;

    if (sc >= 400) {
        auto jsonDoc = safeParse(raw);
        QJsonObject root = jsonDoc.object();
        QString serverMsg = jval(root, "message");
        resp.errorMessage = serverMsg.isEmpty()
            ? "服务器返回错误 (HTTP " + std::to_string(sc) + ")"
            : serverMsg.toStdString();
        return resp;
    }

    auto jsonDoc = safeParse(raw);
    QJsonObject data = jsonDoc.object().value("data").toObject();
    if (data.isEmpty()) { resp.errorMessage = "服务器响应格式异常"; return resp; }

    resp.success = jval_bool(data, "success");
    resp.sessionId = jval_ll(data, "sessionId");
    resp.activeSeats = jval_int(data, "activeSeats");

    return resp;
}

DeactivateResponse LicenseClient::deactivate(const std::string& keyCode) {
    DeactivateResponse resp{};
    int sc = 0;
    std::string raw = winHttpPostWithQuery(baseUrl_, "api/sdk/deactivate", "keyCode=" + keyCode, sc, lastError_);

    if (raw.empty()) { resp.errorMessage = lastError_; return resp; }
    lastStatusCode_ = sc;

    if (sc >= 400) {
        auto jsonDoc = safeParse(raw);
        QJsonObject root = jsonDoc.object();
        QString serverMsg = jval(root, "message");
        resp.errorMessage = serverMsg.isEmpty()
            ? "服务器返回错误 (HTTP " + std::to_string(sc) + ")"
            : serverMsg.toStdString();
        return resp;
    }

    auto jsonDoc = safeParse(raw);
    QJsonObject data = jsonDoc.object().value("data").toObject();

    // [调试] 打印完整响应以便排查
    if (log_) {
        QJsonDocument doc(jsonDoc);
        QString debugMsg = QString("[调试] deactivate 响应: HTTP %1, body=%2, dataKeys=%3, success=%4")
            .arg(sc)
            .arg(doc.toJson(QJsonDocument::Compact))
            .arg(data.keys().join(","))
            .arg(data.value("success").toBool(false));
        log_(debugMsg.toStdString());
    }

    if (data.isEmpty()) { resp.errorMessage = "服务器响应格式异常"; return resp; }

    resp.success = jval_bool(data, "success");
    resp.sessionId = jval_ll(data, "sessionId");
    resp.activeSeats = jval_int(data, "activeSeats");

    return resp;
}

JwksResponse LicenseClient::fetchJwks() {
    JwksResponse resp;
    std::string raw = getRequest(".well-known/jwks.json");
    if (raw.empty()) return resp;

    auto jsonDoc = safeParse(raw);
    QJsonObject root = jsonDoc.object();
    if (root.isEmpty()) return resp;

    QJsonValue keysVal = root.value("keys");
    if (!keysVal.isArray()) return resp;

    QJsonArray keys = keysVal.toArray();
    for (const auto& k : keys) {
        QJsonObject ko = k.toObject();
        JwkKey jwk;
        jwk.kty = ko.value("kty").toString("").toStdString();
        jwk.kid = ko.value("kid").toString("").toStdString();
        jwk.alg = ko.value("alg").toString("").toStdString();
        jwk.n = ko.value("n").toString("").toStdString();
        jwk.e = ko.value("e").toString("AQAB").toStdString();
        resp.keys.push_back(jwk);
    }

    return resp;
}

std::string LicenseClient::getPublicKeyPemForProduct(const std::string& productId) {
    auto jwks = fetchJwks();
    for (auto& k : jwks.keys) {
        if (k.kid == productId) return ""; // Need JWK→PEM conversion, use activate response instead
    }
    return "";
}

// ==================== JWT Verification ====================

std::string LicenseClient::base64UrlDecode(const std::string& input) {
    return jwt_client::base64UrlDecode(input);
}

JwtToken LicenseClient::decodeJwt(const std::string& token) {
    JwtToken result{};
    result.raw = token;

    size_t p1 = token.find('.');
    size_t p2 = (p1 == std::string::npos) ? std::string::npos : token.find('.', p1 + 1);

    if (p1 == std::string::npos || p2 == std::string::npos) {
        throw std::runtime_error("Invalid JWT format");
    }

    std::string payloadRaw = base64UrlDecode(token.substr(p1 + 1, p2 - p1 - 1));
    auto payloadDoc = safeParse(payloadRaw);
    QJsonObject payload = payloadDoc.object();

    result.subject = jval(payload, "sub").toStdString();
    result.issuer = jval(payload, "iss").toStdString();
    result.audience = jval_audience(payload).toStdString();
    result.jwtId = jval(payload, "jti").toStdString();
    result.productId = jval(payload, "product_id").toStdString();
    result.machineHash = jval(payload, "machine_hash").toStdString();
    result.sessionId = std::to_string((long long)jval_ll(payload, "session_id"));

    long long iat = jval_ll(payload, "iat");
    long long exp = jval_ll(payload, "exp");
    result.issuedAt = static_cast<std::time_t>(iat);
    result.expiresAt = static_cast<std::time_t>(exp);

    return result;
}

bool LicenseClient::isTokenExpired(const JwtToken& token) {
    return static_cast<std::time_t>(token.expiresAt) <= std::time(nullptr);
}

// RSA-SHA256 verification using Windows CNG
bool LicenseClient::verifyRsaSha256(const std::string& signingInput,
                                     const std::vector<unsigned char>& signature,
                                     const std::string& publicKeyPem) {
    // 1. Parse PEM to DER (SubjectPublicKeyInfo)
    std::vector<unsigned char> der;
    if (!pemToDerPublicKey(publicKeyPem, der)) return false;

    // 辅助函数：读取 ASN.1 长度（支持多字节长格式：0x81/0x82/0x83/0x84）
    auto readAsn1Length = [&](size_t& pos) -> size_t {
        if (pos >= der.size()) return 0;
        uint8_t lb = der[pos++];
        if (!(lb & 0x80)) return lb;                        // 短格式：0-127
        int numOctets = lb & 0x7F;                           // 长格式：后续字节数
        if (numOctets == 0 || numOctets > 4) return 0;      // 不支持不定长和超大长度
        size_t len = 0;
        for (int i = 0; i < numOctets; i++) {
            if (pos >= der.size()) return 0;
            len = (len << 8) | der[pos++];
        }
        return len;
    };

    // 辅助函数：读取并验证 ASN.1 标签
    auto checkTag = [&](size_t& pos, uint8_t expectedTag) -> bool {
        if (pos >= der.size()) return false;
        if (der[pos] != expectedTag) return false;
        pos++;
        return true;
    };

    // 2. Parse DER SubjectPublicKeyInfo
    // SEQUENCE { AlgorithmIdentifier, BIT STRING { RSAPublicKey } }
    size_t pos = 0;
    if (!checkTag(pos, 0x30)) return false;                  // 外部 SEQUENCE
    size_t outerLen = readAsn1Length(pos);
    if (outerLen == 0) return false;

    // AlgorithmIdentifier SEQUENCE
    if (!checkTag(pos, 0x30)) return false;
    size_t algoLen = readAsn1Length(pos);
    if (algoLen == 0) return false;
    pos += algoLen;                                          // 跳过算法标识

    // BIT STRING（包含 RSAPublicKey）
    if (!checkTag(pos, 0x03)) return false;
    size_t bitStringLen = readAsn1Length(pos);
    if (bitStringLen == 0) return false;
    if (pos >= der.size() || der[pos] != 0x00) return false; // 未使用位数必须为 0
    pos++;

    // RSAPublicKey SEQUENCE
    if (!checkTag(pos, 0x30)) return false;
    size_t rsaLen = readAsn1Length(pos);
    if (rsaLen == 0) return false;

    // Modulus INTEGER
    if (!checkTag(pos, 0x02)) return false;
    size_t modLen = readAsn1Length(pos);
    if (modLen == 0 || pos + modLen > der.size()) return false;
    size_t modStart = pos;
    pos += modLen;

    // Exponent INTEGER
    if (!checkTag(pos, 0x02)) return false;
    size_t expLen = readAsn1Length(pos);
    if (expLen == 0 || pos + expLen > der.size()) return false;
    size_t expStart = pos;

    // 移除前导零（DER INTEGER 在最高位为 1 时会添加 0x00 前缀）
    size_t modEnd = modStart + modLen;
    size_t expEnd = expStart + expLen;
    while (modStart < modEnd && der[modStart] == 0x00) modStart++;
    while (expStart < expEnd && der[expStart] == 0x00) expStart++;
    size_t modActual = modEnd - modStart;
    size_t expActual = expEnd - expStart;

    if (modActual == 0 || expActual == 0) return false;

    // 使用 CNG 验证 JWT 的 RS256 签名；JWT 签名和 BCRYPT_RSAPUBLIC_BLOB 都使用大端字节序。
    ULONG cngBlobLen = sizeof(BCRYPT_RSAKEY_BLOB)
        + static_cast<ULONG>(expActual)
        + static_cast<ULONG>(modActual);
    std::vector<unsigned char> cngBlob(cngBlobLen);
    auto* keyBlob = reinterpret_cast<BCRYPT_RSAKEY_BLOB*>(cngBlob.data());
    keyBlob->Magic = BCRYPT_RSAPUBLIC_MAGIC;
    keyBlob->BitLength = static_cast<ULONG>(modActual * 8);
    keyBlob->cbPublicExp = static_cast<ULONG>(expActual);
    keyBlob->cbModulus = static_cast<ULONG>(modActual);
    keyBlob->cbPrime1 = 0;
    keyBlob->cbPrime2 = 0;

    unsigned char* cngPtr = cngBlob.data() + sizeof(BCRYPT_RSAKEY_BLOB);
    memcpy(cngPtr, der.data() + expStart, expActual);
    cngPtr += expActual;
    memcpy(cngPtr, der.data() + modStart, modActual);

    BCRYPT_ALG_HANDLE hRsaAlg = nullptr;
    BCRYPT_ALG_HANDLE hShaAlg = nullptr;
    BCRYPT_KEY_HANDLE hCngKey = nullptr;
    BCRYPT_HASH_HANDLE hCngHash = nullptr;

    auto cleanup = [&]() {
        if (hCngHash) BCryptDestroyHash(hCngHash);
        if (hCngKey) BCryptDestroyKey(hCngKey);
        if (hShaAlg) BCryptCloseAlgorithmProvider(hShaAlg, 0);
        if (hRsaAlg) BCryptCloseAlgorithmProvider(hRsaAlg, 0);
    };

    NTSTATUS status = BCryptOpenAlgorithmProvider(&hRsaAlg, BCRYPT_RSA_ALGORITHM, nullptr, 0);
    if (status < 0) { cleanup(); return false; }

    status = BCryptImportKeyPair(hRsaAlg, nullptr, BCRYPT_RSAPUBLIC_BLOB, &hCngKey,
                                 cngBlob.data(), cngBlobLen, 0);
    if (status < 0) { cleanup(); return false; }

    status = BCryptOpenAlgorithmProvider(&hShaAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0);
    if (status < 0) { cleanup(); return false; }

    DWORD hashLen = 0;
    DWORD cbData = 0;
    status = BCryptGetProperty(hShaAlg, BCRYPT_HASH_LENGTH,
                               reinterpret_cast<PUCHAR>(&hashLen),
                               sizeof(hashLen), &cbData, 0);
    if (status < 0 || hashLen == 0) { cleanup(); return false; }

    std::vector<unsigned char> hash(hashLen);
    status = BCryptCreateHash(hShaAlg, &hCngHash, nullptr, 0, nullptr, 0, 0);
    if (status < 0) { cleanup(); return false; }

    status = BCryptHashData(hCngHash,
                            const_cast<PUCHAR>(reinterpret_cast<const UCHAR*>(signingInput.data())),
                            static_cast<ULONG>(signingInput.size()), 0);
    if (status < 0) { cleanup(); return false; }

    status = BCryptFinishHash(hCngHash, hash.data(), hashLen, 0);
    if (status < 0) { cleanup(); return false; }

    BCRYPT_PKCS1_PADDING_INFO paddingInfo{};
    paddingInfo.pszAlgId = BCRYPT_SHA256_ALGORITHM;
    status = BCryptVerifySignature(hCngKey, &paddingInfo,
                                   hash.data(), hashLen,
                                   const_cast<PUCHAR>(reinterpret_cast<const UCHAR*>(signature.data())),
                                   static_cast<ULONG>(signature.size()),
                                   BCRYPT_PAD_PKCS1);

    cleanup();
    return status >= 0;
}

JwtToken LicenseClient::verifyJwt(const std::string& token, const std::string& publicKeyPem) {
    JwtToken result = decodeJwt(token);

    size_t p1 = token.find('.');
    size_t p2 = token.find('.', p1 + 1);

    std::string headerB64 = token.substr(0, p1);
    std::string payloadB64 = token.substr(p1 + 1, p2 - p1 - 1);
    std::string signatureB64 = token.substr(p2 + 1);

    std::string signingInput = headerB64 + "." + payloadB64;
    std::vector<unsigned char> sig = b64url_decode(signatureB64);

    // 详细调试信息
    if (log_) {
        char buf[1024];
        snprintf(buf, sizeof(buf), "[调试] JWT token 长度: %zu", token.size());
        log_(buf);
        snprintf(buf, sizeof(buf), "[调试] 签名 base64url 长度: %zu, 解码后: %zu bytes", signatureB64.size(), sig.size());
        log_(buf);
        snprintf(buf, sizeof(buf), "[调试] signingInput 长度: %zu", signingInput.size());
        log_(buf);
        snprintf(buf, sizeof(buf), "[调试] publicKeyPem 长度: %zu", publicKeyPem.size());
        log_(buf);
        // 检查 PEM 格式
        if (publicKeyPem.find("-----BEGIN PUBLIC KEY-----") == std::string::npos) {
            log_("[调试] WARNING: publicKeyPem 不包含 BEGIN PUBLIC KEY 标记!");
            // 输出 PEM 前 80 个字符
            snprintf(buf, sizeof(buf), "[调试] publicKeyPem 前80字符: %.80s", publicKeyPem.c_str());
            log_(buf);
        }
    }

    bool sigValid = verifyRsaSha256(signingInput, sig, publicKeyPem);
    if (!sigValid) {
        std::string detail = "JWT signature verification failed (sig_len=" + std::to_string(sig.size()) +
                            ", signing_input_len=" + std::to_string(signingInput.size()) +
                            ", pem_len=" + std::to_string(publicKeyPem.size()) + ")";
        throw std::runtime_error(detail);
    }

    if (result.issuer != "jwt-license-server") {
        throw std::runtime_error("JWT issuer mismatch");
    }

    if (!expectedProductId_.empty() && result.audience != expectedProductId_) {
        throw std::runtime_error("JWT audience mismatch");
    }

    if (isTokenExpired(result)) {
        throw std::runtime_error("JWT token expired");
    }

    return result;
}

} // namespace jwt_client
