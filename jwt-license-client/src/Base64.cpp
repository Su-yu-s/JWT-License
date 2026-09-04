#include "Base64.h"
#include <stdexcept>

namespace jwt_client {

static const char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64Encode(const std::vector<unsigned char>& data) {
    std::string ret;
    int i = 0;
    unsigned char a3[3] = {0};
    unsigned char a4[4] = {0};

    for (size_t idx = 0; idx < data.size(); ) {
        int w = 0;
        while (w < 3) {
            a3[w++] = (idx < data.size()) ? data[idx++] : 0;
        }
        a4[0] = (a3[0] >> 2) & 0x3F;
        a4[1] = (((a3[0] & 0x03) << 4) | ((a3[1] >> 4) & 0x0F)) & 0x3F;
        a4[2] = (((a3[1] & 0x0F) << 2) | ((a3[2] >> 6) & 0x03)) & 0x3F;
        a4[3] = a3[2] & 0x3F;

        for (int j = 0; j < w + 1; j++) {
            ret += base64_chars[a4[j]];
        }
        while (w < 3) {
            ret += '=';
            w++;
        }
    }
    return ret;
}

std::string base64Decode(const std::string& encoded) {
    unsigned char map256[256];
    for (int i = 0; i < 256; i++) map256[i] = 0;
    for (int i = 0; i < 64; i++) map256[(unsigned char)base64_chars[i]] = i;

    std::string ret;
    size_t len = encoded.size();
    size_t i = 0;

    while (i < len) {
        unsigned char a4[4] = {64, 64, 64, 64};
        int w = 0;
        while (w < 4 && i < len && encoded[i] != '=') {
            a4[w++] = map256[(unsigned char)encoded[i]];
            i++;
        }
        if (w < 2) break;

        ret += static_cast<char>((a4[0] << 2) | ((a4[1] >> 4) & 0x03));
        if (w > 2) ret += static_cast<char>(((a4[1] << 4) & 0xF0) | ((a4[2] >> 2) & 0x0F));
        if (w > 3) ret += static_cast<char>(((a4[2] << 6) & 0xC0) | (a4[3] & 0x3F));
        i += (4 - w);
    }
    return ret;
}

std::string base64UrlEncode(const std::vector<unsigned char>& data) {
    std::string enc = base64Encode(data);
    // Remove padding and replace chars
    std::string ret;
    for (unsigned char c : enc) {
        if (c == '+') ret += '-';
        else if (c == '/') ret += '_';
        else if (c != '=') ret += c;
    }
    return ret;
}

std::string base64UrlDecode(const std::string& encoded) {
    // Restore standard base64
    std::string restored = encoded;
    for (auto& c : restored) {
        if (c == '-') c = '+';
        else if (c == '_') c = '/';
    }
    // Add padding
    while (restored.size() % 4) restored += '=';
    return base64Decode(restored);
}

std::string hexEncode(const std::vector<unsigned char>& data) {
    static const char hex[] = "0123456789abcdef";
    std::string ret;
    ret.reserve(data.size() * 2);
    for (unsigned char c : data) {
        ret += hex[(c >> 4) & 0x0F];
        ret += hex[c & 0x0F];
    }
    return ret;
}

std::vector<unsigned char> hexDecode(const std::string& hex) {
    std::vector<unsigned char> ret;
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        auto fromHex = [](char c) -> unsigned char {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return 0;
        };
        ret.push_back(fromHex(hex[i]) << 4 | fromHex(hex[i + 1]));
    }
    return ret;
}

} // namespace jwt_client
