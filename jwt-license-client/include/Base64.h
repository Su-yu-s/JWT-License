#pragma once

#include <string>
#include <vector>

namespace jwt_client {

// Base64 encoding/decoding (RFC 4648)
std::string base64Encode(const std::vector<unsigned char>& data);
std::string base64Decode(const std::string& encoded);

// Base64URL encoding/decoding (RFC 7515, no padding)
std::string base64UrlEncode(const std::vector<unsigned char>& data);
std::string base64UrlDecode(const std::string& encoded);

// Hex encoding/decoding
std::string hexEncode(const std::vector<unsigned char>& data);
std::vector<unsigned char> hexDecode(const std::string& hex);

} // namespace jwt_client
