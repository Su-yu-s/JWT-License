#pragma once

#include <string>

namespace jwt_client {

// Generate a unique machine hash for this device
// Uses Windows: CPU ID + MAC address + BIOS serial
std::string generateMachineHash();

// Get machine hardware info for display/debug
struct MachineInfo {
    std::string cpuId;
    std::string macAddress;
    std::string biosSerial;
    std::string hostname;
    std::string osVersion;
};
MachineInfo getMachineInfo();

} // namespace jwt_client
