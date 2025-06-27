#include "FTDIEnumerator.h"
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <set>

// Initialize static member
std::string FTDIEnumerator::lastError;

std::vector<FTDIDeviceInfo> FTDIEnumerator::enumerateDevices()
{
    std::vector<FTDIDeviceInfo> devices;

    // Get the number of connected devices
    int deviceCount = getDeviceCount();
    if (deviceCount <= 0)
    {
        return devices; // Empty vector
    }

    // Reserve space for efficiency
    devices.reserve(static_cast<size_t>(deviceCount));

    // Get information for each device
    for (DWORD i = 0; i < static_cast<DWORD>(deviceCount); ++i)
    {
        FTDIDeviceInfo deviceInfo;
        if (getDeviceInfo(i, deviceInfo))
        {
            devices.push_back(std::move(deviceInfo));
        }
    }

    return devices;
}

std::vector<std::string> FTDIEnumerator::getSerialNumbers()
{
    std::vector<std::string> serialNumbers;

    // Get the number of connected devices
    int deviceCount = getDeviceCount();
    if (deviceCount <= 0)
    {
        return serialNumbers; // Empty vector
    }

    // Reserve space for efficiency
    serialNumbers.reserve(static_cast<size_t>(deviceCount));

    // Get serial number for each device
    for (DWORD i = 0; i < static_cast<DWORD>(deviceCount); ++i)
    {
        char serialNumber[64] = {0}; // FTDI serial numbers are typically much shorter

        FT_STATUS status = FT_ListDevices(
            reinterpret_cast<PVOID>(i), serialNumber, FT_LIST_BY_INDEX | FT_OPEN_BY_SERIAL_NUMBER);

        if (status == FT_OK && strlen(serialNumber) > 0)
        {
            serialNumbers.emplace_back(serialNumber);
        }
        else
        {
            setLastError(status);
            // Continue trying other devices even if one fails
        }
    }

    return serialNumbers;
}

int FTDIEnumerator::getDeviceCount()
{
    DWORD     numDevices = 0;
    FT_STATUS status     = FT_ListDevices(&numDevices, nullptr, FT_LIST_NUMBER_ONLY);

    if (status != FT_OK)
    {
        setLastError(status);
        return -1;
    }

    return static_cast<int>(numDevices);
}

bool FTDIEnumerator::getDeviceInfo(DWORD index, FTDIDeviceInfo &deviceInfo)
{
    char serialNumber[64] = {0};
    char description[64]  = {0};

    // Get serial number
    FT_STATUS status = FT_ListDevices(
        reinterpret_cast<PVOID>(index), serialNumber, FT_LIST_BY_INDEX | FT_OPEN_BY_SERIAL_NUMBER);
    if (status != FT_OK)
    {
        setLastError(status);
        return false;
    }

    // Get description
    status = FT_ListDevices(
        reinterpret_cast<PVOID>(index), description, FT_LIST_BY_INDEX | FT_OPEN_BY_DESCRIPTION);
    if (status != FT_OK)
    {
        setLastError(status);
        // Continue even if description fails - we have serial number
    }

    // Fill in the device info
    deviceInfo.index = index;
    safeCopyString(deviceInfo.serialNumber, serialNumber, sizeof(serialNumber));
    safeCopyString(deviceInfo.description, description, sizeof(description));

    // Try to get additional device information
    DWORD     deviceId = 0;
    DWORD     locId    = 0;
    DWORD     flags    = 0;
    DWORD     type     = 0;
    FT_HANDLE handle   = nullptr;

    // Get device info using FT_GetDeviceInfoDetail
    status = FT_GetDeviceInfoDetail(
        index, &flags, &type, &deviceId, &locId, serialNumber, description, &handle);

    if (status == FT_OK)
    {
        deviceInfo.deviceId = deviceId;
        deviceInfo.locId    = locId;
        deviceInfo.flags    = flags;
        deviceInfo.type     = type;

        // Update serial and description with potentially more accurate data
        safeCopyString(deviceInfo.serialNumber, serialNumber, sizeof(serialNumber));
        safeCopyString(deviceInfo.description, description, sizeof(description));
    }
    else
    {
        setLastError(status);
        // Return true anyway since we have basic info
    }

    return true;
}

bool FTDIEnumerator::isDeviceConnected(const std::string &serialNumber)
{
    std::vector<std::string> serialNumbers = getSerialNumbers();

    return std::find(serialNumbers.begin(), serialNumbers.end(), serialNumber) !=
           serialNumbers.end();
}

bool FTDIEnumerator::getDeviceBySerial(const std::string &serialNumber, FTDIDeviceInfo &deviceInfo)
{
    std::vector<FTDIDeviceInfo> devices = enumerateDevices();

    for (const auto &device : devices)
    {
        if (device.serialNumber == serialNumber)
        {
            deviceInfo = device;
            return true;
        }
    }

    setLastError("Device with serial number '" + serialNumber + "' not found");
    return false;
}

void FTDIEnumerator::printDevices()
{
    std::vector<FTDIDeviceInfo> devices = enumerateDevices();

    if (devices.empty())
    {
        std::cout << "No FTDI devices found." << std::endl;
        return;
    }

    std::cout << "Found " << devices.size() << " FTDI device(s):" << std::endl;
    std::cout << std::string(80, '-') << std::endl;

    for (const auto &device : devices)
    {
        std::cout << "Device " << device.index << ":" << std::endl;
        std::cout << "  Serial Number: " << device.serialNumber << std::endl;
        std::cout << "  Description:   " << device.description << std::endl;
        std::cout << "  Device ID:     0x" << std::hex << std::uppercase << std::setw(8)
                  << std::setfill('0') << device.deviceId << std::dec << std::endl;
        std::cout << "  Location ID:   0x" << std::hex << std::uppercase << std::setw(8)
                  << std::setfill('0') << device.locId << std::dec << std::endl;
        std::cout << "  Type:          " << deviceTypeToString(device.type) << " (" << device.type
                  << ")" << std::endl;
        std::cout << "  Flags:         0x" << std::hex << std::uppercase << std::setw(8)
                  << std::setfill('0') << device.flags << std::dec << std::endl;
        std::cout << std::endl;
    }
}

std::vector<std::string> FTDIEnumerator::getUniqueSerialNumbers()
{
    std::vector<std::string> allSerials = getSerialNumbers();
    std::set<std::string>    uniqueBaseSerials;

    for (const std::string &serial : allSerials)
    {
        // Check if it ends with 'A' or 'B' (FT2232H pattern)
        if (!serial.empty() && (serial.back() == 'A' || serial.back() == 'B'))
        {
            // Remove the last character to get base serial
            std::string baseSerial = serial.substr(0, serial.length() - 1);
            // Only add if it starts with SE
            if (baseSerial.substr(0, 2) == "SE")
            {
                uniqueBaseSerials.insert(baseSerial);
            }
        }
    }

    return std::vector<std::string>(uniqueBaseSerials.begin(), uniqueBaseSerials.end());
}

std::string FTDIEnumerator::getLastError() { return lastError; }

std::string FTDIEnumerator::deviceTypeToString(DWORD deviceType)
{
    switch (deviceType)
    {
    case 0:
        return "FT_DEVICE_BM";
    case 1:
        return "FT_DEVICE_AM";
    case 2:
        return "FT_DEVICE_100AX";
    case 3:
        return "FT_DEVICE_UNKNOWN";
    case 4:
        return "FT_DEVICE_2232C";
    case 5:
        return "FT_DEVICE_232R";
    case 6:
        return "FT_DEVICE_2232H";
    case 7:
        return "FT_DEVICE_4232H";
    case 8:
        return "FT_DEVICE_232H";
    case 9:
        return "FT_DEVICE_X_SERIES";
    case 10:
        return "FT_DEVICE_4222H_0";
    case 11:
        return "FT_DEVICE_4222H_1_2";
    case 12:
        return "FT_DEVICE_4222H_3";
    case 13:
        return "FT_DEVICE_4222_PROG";
    case 14:
        return "FT_DEVICE_900";
    case 15:
        return "FT_DEVICE_930";
    case 16:
        return "FT_DEVICE_UMFTPD3A";
    case 17:
        return "FT_DEVICE_2233HP";
    case 18:
        return "FT_DEVICE_4233HP";
    case 19:
        return "FT_DEVICE_2232HP";
    case 20:
        return "FT_DEVICE_4232HP";
    case 21:
        return "FT_DEVICE_233HP";
    case 22:
        return "FT_DEVICE_232HP";
    case 23:
        return "FT_DEVICE_2232HA";
    case 24:
        return "FT_DEVICE_4232HA";
    default:
        return "UNKNOWN_DEVICE_TYPE";
    }
}

void FTDIEnumerator::setLastError(const std::string &error) { lastError = error; }

void FTDIEnumerator::setLastError(FT_STATUS status)
{
    lastError = "FTDI Error: " + std::to_string(status);
}

void FTDIEnumerator::safeCopyString(std::string &dest, const char *src, size_t maxLength)
{
    if (src == nullptr)
    {
        dest.clear();
        return;
    }

    // Find the actual length, but don't exceed maxLength
    size_t len = strnlen(src, maxLength - 1);
    dest.assign(src, len);
}