#include "LicenseClient.h"
#include "MachineInfo.h"
#include <iostream>
#include <string>
#include <ctime>
#include <chrono>

using namespace jwt_client;

static std::string getLine(const std::string& prompt) {
    std::cout << prompt;
    std::string line;
    std::getline(std::cin, line);
    return line;
}

static void printTokenInfo(const JwtToken& token) {
    std::cout << "\n=== JWT Token Info ===\n";
    std::cout << "Subject (keyId):    " << token.subject << "\n";
    std::cout << "Issuer:             " << token.issuer << "\n";
    std::cout << "Audience (productId): " << token.audience << "\n";
    std::cout << "JTI (UUID):         " << token.jwtId << "\n";
    std::cout << "Product ID:         " << token.productId << "\n";
    std::cout << "Machine Hash:       " << token.machineHash << "\n";
    std::cout << "Session ID:         " << token.sessionId << "\n";

    std::time_t now = std::time(nullptr);
    std::cout << "Issued At:          " << std::ctime(&token.issuedAt);
    std::cout << "Expires At:         " << std::ctime(&token.expiresAt);

    double ttl = std::difftime(token.expiresAt, token.issuedAt);
    double remaining = std::difftime(token.expiresAt, now);
    std::cout << "TTL:                " << (ttl / 3600.0) << " hours\n";
    std::cout << "Remaining:          " << (remaining / 3600.0) << " hours\n";
    std::cout << "Expired:            " << (remaining <= 0 ? "YES" : "NO") << "\n\n";
}

static void printResponse(const ActivateResponse& resp) {
    if (!resp.success) {
        std::cout << "Activation failed!\n";
        if (!resp.errorCode.empty()) std::cout << "Error code: " << resp.errorCode << "\n";
        if (!resp.errorMessage.empty()) std::cout << "Message: " << resp.errorMessage << "\n";
        if (!resp.occupied.machineHashPrefix.empty()) {
            std::cout << "Occupied by: " << resp.occupied.machineHashPrefix << "...\n";
            std::cout << "Last seen: " << resp.occupied.lastSeenAt << "\n";
        }
        return;
    }
    std::cout << "Activation successful!\n";
    std::cout << "Token: " << resp.token.substr(0, 60) << "...\n";
    std::cout << "Session ID: " << resp.sessionId << "\n";
    std::cout << "Public Key received (" << resp.publicKeyPem.size() << " bytes)\n\n";
}

static void demoActivate(LicenseClient& client) {
    std::cout << "\n--- Activate ---\n";
    std::string keyCode = getLine("Enter license key code: ");
    std::string machineHash = generateMachineHash();
    std::cout << "Machine hash: " << machineHash << "\n";

    ActivateResponse resp = client.activate(keyCode, machineHash);
    printResponse(resp);

    if (resp.success && !resp.publicKeyPem.empty()) {
        try {
            JwtToken token = client.verifyJwt(resp.token, resp.publicKeyPem);
            printTokenInfo(token);
        } catch (const std::exception& e) {
            std::cout << "JWT verification failed: " << e.what() << "\n";
        }
    }
}

static void demoHeartbeat(LicenseClient& client) {
    std::cout << "\n--- Heartbeat ---\n";
    std::string keyCode = getLine("Enter license key code: ");

    HeartbeatResponse resp = client.heartbeat(keyCode);
    if (!resp.success) {
        std::cout << "Heartbeat failed!\n";
        return;
    }
    std::cout << "Heartbeat OK. New token received.\n";
    std::cout << "Session: " << resp.sessionId << "\n";

    if (!resp.publicKeyPem.empty()) {
        try {
            JwtToken token = client.verifyJwt(resp.token, resp.publicKeyPem);
            printTokenInfo(token);
        } catch (const std::exception& e) {
            std::cout << "JWT verification failed: " << e.what() << "\n";
        }
    }
}

static void demoOnlineCheck(LicenseClient& client) {
    std::cout << "\n--- Online Check ---\n";
    std::string keyCode = getLine("Enter license key code: ");

    OnlineCheckResponse resp = client.onlineCheck(keyCode);
    if (!resp.success) {
        std::cout << "Online check failed!\n";
        return;
    }
    std::cout << "Device is online. Session: " << resp.sessionId << "\n";
}

static void demoDeactivate(LicenseClient& client) {
    std::cout << "\n--- Deactivate ---\n";
    std::string keyCode = getLine("Enter license key code: ");

    DeactivateResponse resp = client.deactivate(keyCode);
    if (!resp.success) {
        std::cout << "Deactivation failed!\n";
        return;
    }
    std::cout << "Deactivated. Session: " << resp.sessionId << "\n";
}

static void demoJwks(LicenseClient& client) {
    std::cout << "\n--- JWKS (Public Keys) ---\n";
    auto jwks = client.fetchJwks();
    std::cout << "Found " << jwks.keys.size() << " product(s):\n";
    for (auto& k : jwks.keys) {
        std::cout << "  kid=" << k.kid << " alg=" << k.alg
                  << " n=" << k.n.substr(0, 40) << "...\n";
    }
}

static void demoDecode(LicenseClient& client) {
    std::cout << "\n--- Decode JWT (no verification) ---\n";
    std::string token = getLine("Enter JWT token: ");

    try {
        JwtToken decoded = client.decodeJwt(token);
        printTokenInfo(decoded);
    } catch (const std::exception& e) {
        std::cout << "Decode failed: " << e.what() << "\n";
    }
}

static void demoMachineInfo() {
    std::cout << "\n--- Machine Info ---\n";
    MachineInfo info = getMachineInfo();
    std::cout << "CPU ID:       " << info.cpuId << "\n";
    std::cout << "MAC Address:  " << info.macAddress << "\n";
    std::cout << "BIOS Serial:  " << info.biosSerial << "\n";
    std::cout << "Hostname:     " << info.hostname << "\n";
    std::cout << "OS Version:   " << info.osVersion << "\n";
    std::cout << "Machine Hash: " << generateMachineHash() << "\n";
}

void printUsage() {
    std::cout << "\n=== JWT License Client ===\n\n";
    std::cout << "Available commands:\n";
    std::cout << "  1.  Activate      - Activate license with key code\n";
    std::cout << "  2.  Heartbeat     - Send heartbeat, refresh token\n";
    std::cout << "  3.  OnlineCheck   - Check if device is online\n";
    std::cout << "  4.  Deactivate    - Deactivate/release license\n";
    std::cout << "  5.  Jwks          - Fetch all product public keys\n";
    std::cout << "  6.  DecodeJwt     - Decode JWT (no verification)\n";
    std::cout << "  7.  MachineInfo   - Show machine hardware info\n";
    std::cout << "  8.  ChangeServer  - Change server URL\n";
    std::cout << "  0.  Exit\n";
}

int main(int argc, char* argv[]) {
    std::string serverUrl = "http://localhost:8080";

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if ((arg == "--server" || arg == "-s") && i + 1 < argc) {
            serverUrl = argv[++i];
        } else if ((arg == "--keyCode" || arg == "-k") && i + 1 < argc) {
            // Quick activate mode
            LicenseClient client(serverUrl);
            std::string machineHash = generateMachineHash();
            ActivateResponse resp = client.activate(argv[++i], machineHash);
            printResponse(resp);

            if (resp.success && !resp.publicKeyPem.empty()) {
                try {
                    JwtToken token = client.verifyJwt(resp.token, resp.publicKeyPem);
                    printTokenInfo(token);
                } catch (const std::exception& e) {
                    std::cerr << "Verification failed: " << e.what() << "\n";
                    return 1;
                }
                return 0;
            }
            return 1;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: jwt_license_client [--server URL] [--keyCode KEYCODE]\n";
            std::cout << "  --server, -s  Server URL (default: http://localhost:8080)\n";
            std::cout << "  --keyCode, -k License key code for quick activation\n";
            std::cout << "  --help, -h    Show this help\n";
            return 0;
        }
    }

    LicenseClient client(serverUrl, [](const std::string& msg) {
        std::cout << "[LOG] " << msg << "\n";
    });

    std::cout << "JWT License Client v1.0\n";
    std::cout << "Connected to: " << serverUrl << "\n\n";

    std::string productId = getLine("Enter expected product ID (audience, e.g. demo_app-v1): ");
    client.setExpectedProductId(productId);

    printUsage();

    while (true) {
        std::cout << "\nSelect command: ";
        std::string input;
        std::getline(std::cin, input);

        if (input == "1") demoActivate(client);
        else if (input == "2") demoHeartbeat(client);
        else if (input == "3") demoOnlineCheck(client);
        else if (input == "4") demoDeactivate(client);
        else if (input == "5") demoJwks(client);
        else if (input == "6") demoDecode(client);
        else if (input == "7") demoMachineInfo();
        else if (input == "8") {
            serverUrl = getLine("Enter new server URL: ");
            client = LicenseClient(serverUrl);
            std::cout << "Server changed to: " << serverUrl << "\n";
        }
        else if (input == "0") {
            std::cout << "Bye!\n";
            break;
        }
        else {
            std::cout << "Unknown command.\n";
            printUsage();
        }
    }

    return 0;
}
