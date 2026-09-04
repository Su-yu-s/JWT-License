#include "MachineInfo.h"

#ifdef _WIN32
#include <windows.h>
#include <iphlpapi.h>
#include <wincrypt.h>
#include <intrin.h>
#include <stdio.h>

// Define missing constants for older SDKs
#ifndef QueryTypeStandard
#define QueryTypeStandard 0
#endif
#endif

#include <sstream>
#include <algorithm>
#include <iomanip>

namespace jwt_client {

static void cpuid(int info[], int function_id) {
#ifdef _WIN32
    __cpuid(info, function_id);
#else
    (void)info; (void)function_id;
#endif
}

std::string generateMachineHash() {
#ifdef _WIN32
    std::ostringstream oss;

    // 1. CPU ID
    int cpuInfo[4] = {0};
    cpuid(cpuInfo, 0);
    oss << cpuInfo[0] << cpuInfo[1] << cpuInfo[2] << cpuInfo[3];

    cpuid(cpuInfo, 0x80000002);
    oss << cpuInfo[0] << cpuInfo[1] << cpuInfo[2] << cpuInfo[3];
    cpuid(cpuInfo, 0x80000003);
    oss << cpuInfo[0] << cpuInfo[1] << cpuInfo[2] << cpuInfo[3];
    cpuid(cpuInfo, 0x80000004);
    oss << cpuInfo[0] << cpuInfo[1] << cpuInfo[2] << cpuInfo[3];

    // 2. MAC address
    ULONG outBufLen = 0;
    GetAdaptersInfo(NULL, &outBufLen);
    if (outBufLen > 0) {
        PIP_ADAPTER_INFO pAdapterInfo = (PIP_ADAPTER_INFO)new BYTE[outBufLen];
        if (GetAdaptersInfo(pAdapterInfo, &outBufLen) == NO_ERROR) {
            for (PIP_ADAPTER_INFO pAdapter = pAdapterInfo; pAdapter; pAdapter = pAdapter->Next) {
                oss << std::hex << std::uppercase;
                for (UINT i = 0; i < pAdapter->AddressLength; i++) {
                    oss << std::setw(2) << std::setfill('0') << (int)pAdapter->Address[i];
                }
                oss << std::dec;
            }
        }
        delete[] pAdapterInfo;
    }

    // 3. BIOS Serial
    char biosSerial[256] = {0};
    if (GetSystemFirmwareTable('RSMB', 0, biosSerial, sizeof(biosSerial))) {
        oss << biosSerial;
    }

    // 4. Hostname
    char hostname[256] = {0};
    DWORD hostnameLen = sizeof(hostname);
    GetComputerNameA(hostname, &hostnameLen);
    oss << hostname;

    // 5. Disk serial
    HANDLE hDevice = CreateFileA("\\\\.\\PhysicalDrive0",
                                 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice != INVALID_HANDLE_VALUE) {
        STORAGE_PROPERTY_QUERY query;
        ZeroMemory(&query, sizeof(query));
        query.PropertyId = StorageDeviceProperty;
        query.QueryType = (STORAGE_QUERY_TYPE)QueryTypeStandard;
        BYTE diskInfo[1024] = {0};
        DWORD bytesReturned = 0;
        if (DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
                            &query, sizeof(query),
                            diskInfo, sizeof(diskInfo),
                            &bytesReturned, NULL)) {
            STORAGE_DEVICE_DESCRIPTOR* desc = (STORAGE_DEVICE_DESCRIPTOR*)diskInfo;
            if (desc->SerialNumberOffset > 0) {
                oss << (const char*)(diskInfo + desc->SerialNumberOffset);
            }
        }
        CloseHandle(hDevice);
    }

    std::string raw = oss.str();

    // SHA-256
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    BYTE hashBuf[32] = {0};
    DWORD hashLen = 32;

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
        return "hash-error";
    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        return "hash-error";
    }
    CryptHashData(hHash, (const BYTE*)raw.c_str(), (DWORD)raw.size(), 0);
    CryptGetHashParam(hHash, HP_HASHVAL, hashBuf, &hashLen, 0);
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);

    std::string result;
    result.reserve(64);
    for (int i = 0; i < 32; i++) {
        char buf[3];
        snprintf(buf, sizeof(buf), "%02x", hashBuf[i]);
        result += buf;
    }
    return result;
#else
    return "platform-not-supported";
#endif
}

MachineInfo getMachineInfo() {
    MachineInfo info;
    info.osVersion = "Windows";

#ifdef _WIN32
    int cpuInfo[4] = {0};
    cpuid(cpuInfo, 0);
    info.cpuId = std::to_string(cpuInfo[0]) + " " + std::to_string(cpuInfo[1])
               + " " + std::to_string(cpuInfo[2]) + " " + std::to_string(cpuInfo[3]);

    ULONG outBufLen = 0;
    GetAdaptersInfo(NULL, &outBufLen);
    if (outBufLen > 0) {
        PIP_ADAPTER_INFO pAdapterInfo = (PIP_ADAPTER_INFO)new BYTE[outBufLen];
        if (GetAdaptersInfo(pAdapterInfo, &outBufLen) == NO_ERROR) {
            for (PIP_ADAPTER_INFO pAdapter = pAdapterInfo; pAdapter; pAdapter = pAdapter->Next) {
                std::ostringstream mac;
                for (UINT i = 0; i < pAdapter->AddressLength; i++) {
                    if (i) mac << ":";
                    mac << std::hex << std::setw(2) << std::setfill('0') << (int)pAdapter->Address[i];
                }
                info.macAddress = mac.str();
                break;
            }
        }
        delete[] pAdapterInfo;
    }

    char biosSerial[256] = {0};
    if (GetSystemFirmwareTable('RSMB', 0, biosSerial, sizeof(biosSerial))) {
        info.biosSerial = biosSerial;
    }

    char hostname[256] = {0};
    DWORD hostnameLen = sizeof(hostname);
    GetComputerNameA(hostname, &hostnameLen);
    info.hostname = hostname;

    OSVERSIONINFOA osvi = {0};
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (GetVersionExA(&osvi)) {
        info.osVersion = "Windows " + std::to_string(osvi.dwMajorVersion) + "."
                       + std::to_string(osvi.dwMinorVersion) + " (build "
                       + std::to_string(osvi.dwBuildNumber) + ")";
    }
#endif

    return info;
}

} // namespace jwt_client
