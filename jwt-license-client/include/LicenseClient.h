#pragma once

#include <string>
#include <vector>
#include <functional>
#include <ctime>

namespace jwt_client {

// JWT token claims
struct JwtToken {
    std::string raw;
    std::string subject;      // licenseKeyId
    std::string issuer;       // "jwt-license-server"
    std::string audience;     // productId
    std::string jwtId;        // UUID
    std::time_t issuedAt;
    std::time_t expiresAt;
    std::string productId;
    std::string machineHash;
    std::string sessionId;
};

// Activation response from server
struct ActivateResponse {
    bool success = false;
    std::string token;
    std::string issuedAt;
    std::string expiresAt;
    long long sessionId = 0;
    int capacity = 0;
    int activeSeats = 0;
    std::string publicKeyPem;
    std::string issuer;

    struct OccupiedInfo {
        long long sessionId = 0;
        std::string machineHashPrefix;
        std::string lastSeenAt;
    };
    std::string errorCode;
    std::string errorMessage;
    OccupiedInfo occupied;
};

// Heartbeat response
struct HeartbeatResponse {
    bool success = false;
    std::string token;
    std::string expiresAt;
    long long sessionId = 0;
    int activeSeats = 0;
    std::string publicKeyPem;
    std::string issuer;
    std::string errorMessage;
};

// Online check response
struct OnlineCheckResponse {
    bool success = false;
    long long sessionId = 0;
    int activeSeats = 0;
    std::string errorMessage;
};

// Deactivate response
struct DeactivateResponse {
    bool success = false;
    long long sessionId = 0;
    int activeSeats = 0;
    std::string errorMessage;
};

// JWK key
struct JwkKey {
    std::string kty;
    std::string kid;
    std::string alg;
    std::string n;    // modulus
    std::string e;    // exponent
};

// JWKS response
struct JwksResponse {
    std::vector<JwkKey> keys;
};

class LicenseClient {
public:
    using ProgressCallback = std::function<void(const std::string&)>;

    explicit LicenseClient(const std::string& serverUrl, ProgressCallback log = nullptr);

    // Set expected product ID for JWT audience verification
    void setExpectedProductId(const std::string& productId);

    // === SDK API Methods ===

    // 1. Activate: obtain JWT token
    ActivateResponse activate(const std::string& keyCode,
                              const std::string& machineHash,
                              const std::string& sdkVersion = "cpp-v1.0");

    // 2. Heartbeat: refresh JWT token
    HeartbeatResponse heartbeat(const std::string& keyCode);

    // 3. Online check: verify without refreshing
    OnlineCheckResponse onlineCheck(const std::string& keyCode);

    // 4. Deactivate: release license
    DeactivateResponse deactivate(const std::string& keyCode);

    // === JWT Verification ===

    // 5. Verify JWT token signature and claims
    JwtToken verifyJwt(const std::string& token, const std::string& publicKeyPem);

    // 6. Check if token is expired
    bool isTokenExpired(const JwtToken& token);

    // 7. Decode JWT without verification (for debugging)
    JwtToken decodeJwt(const std::string& token);

    // === Key Management ===

    // 8. Fetch JWKS (all product public keys in JWK format)
    JwksResponse fetchJwks();

    // 9. Get public key PEM for specific product
    std::string getPublicKeyPemForProduct(const std::string& productId);

    // === Utilities ===

    // Get last HTTP status code
    int getLastStatusCode() const { return lastStatusCode_; }

    // Get last error message
    std::string getLastError() const { return lastError_; }

private:
    std::string baseUrl_;
    std::string expectedProductId_;
    ProgressCallback log_;
    int lastStatusCode_ = 0;
    std::string lastError_;

    // HTTP helpers
    std::string postRequest(const std::string& path, const std::string& body);
    std::string getRequest(const std::string& path);

    // JWT helpers
    std::string base64UrlDecode(const std::string& input);
    std::string trim(const std::string& s);

    // Crypto helpers — RSA-SHA256 verification using Windows CNG
    bool verifyRsaSha256(const std::string& signingInput,
                         const std::vector<unsigned char>& signature,
                         const std::string& publicKeyPem);
};

} // namespace jwt_client
