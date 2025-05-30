#ifndef FTDI_ENUMERATOR_H
#define FTDI_ENUMERATOR_H

#include <memory>
#include <string>
#include <vector>

// FTDI D2XX library headers - adjust path as needed
#include "ftd2xx.h"

/**
 * @brief Structure to hold information about an FTDI device
 */
struct FTDIDeviceInfo
{
    DWORD       index;        ///< Device index in the system
    std::string serialNumber; ///< Device serial number
    std::string description;  ///< Device description
    DWORD       deviceId;     ///< Device ID (VID/PID)
    DWORD       locId;        ///< Location ID
    DWORD       flags;        ///< Device flags
    DWORD       type;         ///< Device type

    // Default constructor
    FTDIDeviceInfo() : index(0), deviceId(0), locId(0), flags(0), type(0) {}

    // Constructor with parameters
    FTDIDeviceInfo(DWORD              idx,
                   const std::string &serial,
                   const std::string &desc,
                   DWORD              devId,
                   DWORD              location,
                   DWORD              devFlags,
                   DWORD              devType)
        : index(idx), serialNumber(serial), description(desc), deviceId(devId), locId(location),
          flags(devFlags), type(devType)
    {
    }
};

/**
 * @brief FTDI Device Enumerator Class
 *
 * This class provides functionality to enumerate all connected FTDI devices
 * and retrieve their information including serial numbers, descriptions, and
 * other device properties using only the D2XX library and standard C++ libraries.
 */
class FTDIEnumerator
{
  public:
    /**
     * @brief Enumerate all connected FTDI devices
     * @return Vector of FTDIDeviceInfo structures containing device information
     */
    static std::vector<FTDIDeviceInfo> enumerateDevices();

    /**
     * @brief Get serial numbers of all connected FTDI devices
     * @return Vector of serial number strings
     */
    static std::vector<std::string> getSerialNumbers();

    /**
     * @brief Get base serial numbers from FT2232H devices (Knokke V1 scanners)
     *
     * FT2232H devices appear as two separate devices with serials ending in 'A' and 'B'.
     * This method extracts the base serial number (without the A/B suffix) and returns
     * unique base serial numbers for Knokke V1 film scanners.
     *
     * @return Vector of unique base serial numbers (without A/B suffix)
     *
     * Example: If devices "SE9258YRA" and "SE9258YRB" are connected,
     *          this will return {"SE9258YR"}
     */
    static std::vector<std::string> getUniqueSerialNumbers();

    /**
     * @brief Get the number of connected FTDI devices
     * @return Number of devices found, or -1 on error
     */
    static int getDeviceCount();

    /**
     * @brief Get device information by index
     * @param index Device index (0-based)
     * @param deviceInfo Output parameter to store device information
     * @return true if successful, false on error
     */
    static bool getDeviceInfo(DWORD index, FTDIDeviceInfo &deviceInfo);

    /**
     * @brief Check if a device with the given serial number is connected
     * @param serialNumber Serial number to search for
     * @return true if device is found, false otherwise
     */
    static bool isDeviceConnected(const std::string &serialNumber);

    /**
     * @brief Get device information by serial number
     * @param serialNumber Serial number to search for
     * @param deviceInfo Output parameter to store device information
     * @return true if device is found and info retrieved, false otherwise
     */
    static bool getDeviceBySerial(const std::string &serialNumber, FTDIDeviceInfo &deviceInfo);

    /**
     * @brief Print information about all connected devices to stdout
     */
    static void printDevices();

    /**
     * @brief Get the last error message
     * @return Last error message
     */
    static std::string getLastError();

    /**
     * @brief Convert device type to string
     * @param deviceType Device type value
     * @return String representation of device type
     */
    static std::string deviceTypeToString(DWORD deviceType);

  private:
    static std::string lastError;

    static void setLastError(const std::string &error);
    static void setLastError(FT_STATUS status);
    static void safeCopyString(std::string &dest, const char *src, size_t maxLength);
};

#endif // FTDI_ENUMERATOR_H