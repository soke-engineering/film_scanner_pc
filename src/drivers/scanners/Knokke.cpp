#include "Knokke.h"
#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>

Knokke::Knokke()
    : m_context(nullptr), m_deviceHandle(nullptr), m_connected(false), m_streaming(false),
      m_threadRunning(false), m_frameNumber(0)
{
    m_frameBuffer.reserve(FRAME_BYTES);
}

Knokke::~Knokke()
{
    disconnect();
    if (m_context)
    {
        libusb_exit(m_context);
    }
}

Knokke::Error Knokke::initialize()
{
    int result = libusb_init(&m_context);
    if (result < 0)
    {
        handleError(Error::USB_ERROR,
                    "Failed to initialize libusb: " + std::string(libusb_error_name(result)));
        return Error::USB_ERROR;
    }

    // Set debug level to reduce verbosity
    libusb_set_option(m_context, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING);

    return Error::SUCCESS;
}

Knokke::Error Knokke::connect()
{
    if (m_connected)
    {
        return Error::SUCCESS;
    }

    if (!m_context)
    {
        Error result = initialize();
        if (result != Error::SUCCESS)
        {
            return result;
        }
    }

    if (!findDevice())
    {
        return Error::DEVICE_NOT_FOUND;
    }

    Error result = claimInterfaces();
    if (result != Error::SUCCESS)
    {
        libusb_close(m_deviceHandle);
        m_deviceHandle = nullptr;
        return result;
    }

    m_connected = true;
    return Error::SUCCESS;
}

void Knokke::disconnect()
{
    if (m_streaming)
    {
        stopStreaming();
    }

    if (m_deviceHandle)
    {
        releaseInterfaces();
        libusb_close(m_deviceHandle);
        m_deviceHandle = nullptr;
    }

    m_connected = false;
}

bool Knokke::isConnected() const { return m_connected; }

Knokke::Error Knokke::getExposureTime(uint32_t &exposureTime)
{
    if (!m_connected)
    {
        return Error::DEVICE_NOT_CONNECTED;
    }

    uint8_t data[4];
    Error   result = performControlTransfer(UVC_REQUEST_TYPE_CLASS,
                                          UVC_GET_CUR,
                                          UVC_EXPOSURE_CONTROL,
                                          UVC_CAMERA_TERMINAL,
                                          data,
                                          sizeof(data));

    if (result == Error::SUCCESS)
    {
        exposureTime = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    }

    return result;
}

Knokke::Error Knokke::setExposureTime(uint32_t exposureTime)
{
    if (!m_connected)
    {
        return Error::DEVICE_NOT_CONNECTED;
    }

    uint8_t data[4];
    data[0] = exposureTime & 0xFF;
    data[1] = (exposureTime >> 8) & 0xFF;
    data[2] = (exposureTime >> 16) & 0xFF;
    data[3] = (exposureTime >> 24) & 0xFF;

    return performControlTransfer(UVC_REQUEST_TYPE_CLASS_OUT,
                                  UVC_SET_CUR,
                                  UVC_EXPOSURE_CONTROL,
                                  UVC_CAMERA_TERMINAL,
                                  data,
                                  sizeof(data));
}

Knokke::Error Knokke::getGain(uint16_t &gain)
{
    if (!m_connected)
    {
        return Error::DEVICE_NOT_CONNECTED;
    }

    uint8_t data[2];
    Error   result = performControlTransfer(UVC_REQUEST_TYPE_CLASS,
                                          UVC_GET_CUR,
                                          UVC_GAIN_CONTROL,
                                          UVC_CAMERA_TERMINAL,
                                          data,
                                          sizeof(data));

    if (result == Error::SUCCESS)
    {
        gain = data[0] | (data[1] << 8);
    }

    return result;
}

Knokke::Error Knokke::setGain(uint16_t gain)
{
    if (!m_connected)
    {
        return Error::DEVICE_NOT_CONNECTED;
    }

    uint8_t data[2];
    data[0] = gain & 0xFF;
    data[1] = (gain >> 8) & 0xFF;

    return performControlTransfer(UVC_REQUEST_TYPE_CLASS_OUT,
                                  UVC_SET_CUR,
                                  UVC_GAIN_CONTROL,
                                  UVC_CAMERA_TERMINAL,
                                  data,
                                  sizeof(data));
}

Knokke::Error Knokke::getBacklight(BacklightParams &backlight)
{
    if (!m_connected)
    {
        return Error::DEVICE_NOT_CONNECTED;
    }

    uint8_t data[6];
    Error   result = performControlTransfer(UVC_REQUEST_TYPE_CLASS,
                                          UVC_GET_CUR,
                                          UVC_BACKLIGHT_CONTROL,
                                          UVC_EXTENSION_UNIT,
                                          data,
                                          sizeof(data));

    if (result == Error::SUCCESS)
    {
        backlight.red   = data[0] | (data[1] << 8);
        backlight.green = data[2] | (data[3] << 8);
        backlight.blue  = data[4] | (data[5] << 8);
    }

    return result;
}

Knokke::Error Knokke::setBacklight(const BacklightParams &backlight)
{
    if (!m_connected)
    {
        return Error::DEVICE_NOT_CONNECTED;
    }

    uint8_t data[6];
    data[0] = backlight.red & 0xFF;
    data[1] = (backlight.red >> 8) & 0xFF;
    data[2] = backlight.green & 0xFF;
    data[3] = (backlight.green >> 8) & 0xFF;
    data[4] = backlight.blue & 0xFF;
    data[5] = (backlight.blue >> 8) & 0xFF;

    return performControlTransfer(UVC_REQUEST_TYPE_CLASS_OUT,
                                  UVC_SET_CUR,
                                  UVC_BACKLIGHT_CONTROL,
                                  UVC_EXTENSION_UNIT,
                                  data,
                                  sizeof(data));
}

Knokke::Error Knokke::setBacklightChannel(char channel, uint16_t value)
{
    if (!m_connected)
    {
        return Error::DEVICE_NOT_CONNECTED;
    }

    // Get current backlight values first
    BacklightParams current;
    Error           result = getBacklight(current);
    if (result != Error::SUCCESS)
    {
        return result;
    }

    // Update the specified channel
    switch (channel)
    {
    case 'r':
    case 'R':
        current.red = value;
        break;
    case 'g':
    case 'G':
        current.green = value;
        break;
    case 'b':
    case 'B':
        current.blue = value;
        break;
    default:
        return Error::INVALID_PARAMETER;
    }

    return setBacklight(current);
}

Knokke::Error Knokke::getMotorSpeed(int32_t &speed)
{
    if (!m_connected)
    {
        return Error::DEVICE_NOT_CONNECTED;
    }

    uint8_t data[4];
    Error   result = performControlTransfer(UVC_REQUEST_TYPE_CLASS,
                                          UVC_GET_CUR,
                                          UVC_MOTOR_SPEED_CONTROL,
                                          UVC_EXTENSION_UNIT,
                                          data,
                                          sizeof(data));

    if (result == Error::SUCCESS)
    {
        speed = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    }

    return result;
}

Knokke::Error Knokke::setMotorSpeed(int32_t speed)
{
    if (!m_connected)
    {
        return Error::DEVICE_NOT_CONNECTED;
    }

    uint8_t data[4];
    data[0] = speed & 0xFF;
    data[1] = (speed >> 8) & 0xFF;
    data[2] = (speed >> 16) & 0xFF;
    data[3] = (speed >> 24) & 0xFF;

    return performControlTransfer(UVC_REQUEST_TYPE_CLASS_OUT,
                                  UVC_SET_CUR,
                                  UVC_MOTOR_SPEED_CONTROL,
                                  UVC_EXTENSION_UNIT,
                                  data,
                                  sizeof(data));
}

Knokke::Error Knokke::getParameters(ScannerParams &params)
{
    Error result = getExposureTime(params.exposure_time);
    if (result != Error::SUCCESS)
        return result;

    result = getGain(params.gain);
    if (result != Error::SUCCESS)
        return result;

    result = getBacklight(params.backlight);
    if (result != Error::SUCCESS)
        return result;

    result = getMotorSpeed(params.motor_speed);
    if (result != Error::SUCCESS)
        return result;

    return Error::SUCCESS;
}

Knokke::Error Knokke::setParameters(const ScannerParams &params)
{
    Error result = setExposureTime(params.exposure_time);
    if (result != Error::SUCCESS)
        return result;

    result = setGain(params.gain);
    if (result != Error::SUCCESS)
        return result;

    result = setBacklight(params.backlight);
    if (result != Error::SUCCESS)
        return result;

    result = setMotorSpeed(params.motor_speed);
    if (result != Error::SUCCESS)
        return result;

    return Error::SUCCESS;
}

Knokke::Error Knokke::startStreaming()
{
    if (!m_connected)
    {
        return Error::DEVICE_NOT_CONNECTED;
    }

    if (m_streaming)
    {
        return Error::STREAMING_ALREADY_STARTED;
    }

    Error result = sendProbeCommit();
    if (result != Error::SUCCESS)
    {
        return result;
    }

    m_streaming     = true;
    m_threadRunning = true;

    // Start capture thread
    m_captureThread = std::make_unique<std::thread>(&Knokke::captureThreadFunction, this);

    return Error::SUCCESS;
}

Knokke::Error Knokke::stopStreaming()
{
    if (!m_streaming)
    {
        return Error::SUCCESS;
    }

    m_streaming     = false;
    m_threadRunning = false;

    // Wait for thread to finish
    if (m_captureThread && m_captureThread->joinable())
    {
        m_captureThread->join();
    }

    m_captureThread.reset();

    return Error::SUCCESS;
}

bool Knokke::isStreaming() const { return m_streaming; }

void Knokke::setFrameCallback(FrameCallback callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_frameCallback = callback;
}

void Knokke::setErrorCallback(ErrorCallback callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_errorCallback = callback;
}

Knokke::Error Knokke::captureFrame(uint8_t *frameData, size_t frameSize, int timeoutMs)
{
    if (!m_connected)
    {
        return Error::DEVICE_NOT_CONNECTED;
    }

    if (frameSize < FRAME_BYTES)
    {
        return Error::INVALID_PARAMETER;
    }

    const int            payloadSize = 32768;
    std::vector<uint8_t> payload(payloadSize);
    std::vector<uint8_t> frame;
    frame.reserve(FRAME_BYTES);

    const uint8_t UVC_HEADER_EOF = 1u << 1;
    bool          gotEof         = false;
    auto          startTime      = std::chrono::steady_clock::now();

    while (!gotEof && frame.size() < FRAME_BYTES)
    {
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsed =
            std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime);
        if (elapsed.count() > timeoutMs)
        {
            return Error::CONTROL_TRANSFER_FAILED;
        }

        int transferred = 0;
        int result      = libusb_bulk_transfer(
            m_deviceHandle, BULK_EP_IN, payload.data(), payloadSize, &transferred, 200);

        if (result == LIBUSB_ERROR_TIMEOUT)
        {
            continue;
        }
        if (result < 0)
        {
            handleError(Error::USB_ERROR,
                        "Bulk transfer error: " + std::string(libusb_error_name(result)));
            return Error::USB_ERROR;
        }
        if (transferred <= 0)
        {
            continue;
        }

        // Parse UVC payload header
        if (transferred < 2)
            continue;

        const uint8_t *p         = payload.data();
        const uint8_t  headerLen = p[0];
        const uint8_t  headerBfh = p[1];

        if (headerLen > transferred)
            continue;

        const uint8_t *img      = p + headerLen;
        const int      imgBytes = transferred - headerLen;

        if (imgBytes > 0)
        {
            size_t spaceLeft = FRAME_BYTES - frame.size();
            size_t toCopy    = std::min(static_cast<size_t>(imgBytes), spaceLeft);
            frame.insert(frame.end(), img, img + toCopy);
        }

        if (headerBfh & UVC_HEADER_EOF)
        {
            gotEof = true;
        }
    }

    if (frame.size() < FRAME_BYTES)
    {
        return Error::CONTROL_TRANSFER_FAILED;
    }

    std::memcpy(frameData, frame.data(), FRAME_BYTES);
    return Error::SUCCESS;
}

Knokke::Error Knokke::captureFrames(int numFrames, FrameCallback frameCallback)
{
    if (!m_connected)
    {
        return Error::DEVICE_NOT_CONNECTED;
    }

    for (int i = 0; i < numFrames; ++i)
    {
        std::vector<uint8_t> frame(FRAME_BYTES);
        Error                result = captureFrame(frame.data(), FRAME_BYTES);
        if (result != Error::SUCCESS)
        {
            return result;
        }

        frameCallback(frame.data(), FRAME_BYTES, i);
    }

    return Error::SUCCESS;
}

std::string Knokke::getErrorMessage(Error error)
{
    switch (error)
    {
    case Error::SUCCESS:
        return "Success";
    case Error::DEVICE_NOT_FOUND:
        return "Device not found";
    case Error::DEVICE_OPEN_FAILED:
        return "Failed to open device";
    case Error::DEVICE_NOT_CONNECTED:
        return "Device not connected";
    case Error::CONTROL_TRANSFER_FAILED:
        return "Control transfer failed";
    case Error::STREAMING_NOT_STARTED:
        return "Streaming not started";
    case Error::STREAMING_ALREADY_STARTED:
        return "Streaming already started";
    case Error::THREAD_CREATION_FAILED:
        return "Thread creation failed";
    case Error::INVALID_PARAMETER:
        return "Invalid parameter";
    case Error::USB_ERROR:
        return "USB error";
    case Error::UNKNOWN_ERROR:
    default:
        return "Unknown error";
    }
}

std::string Knokke::getDeviceInfo() const
{
    if (!m_connected)
    {
        return "Device not connected";
    }

    // Get actual device descriptor
    libusb_device           *device = libusb_get_device(m_deviceHandle);
    libusb_device_descriptor desc;

    if (libusb_get_device_descriptor(device, &desc) == 0)
    {
        std::ostringstream oss;
        oss << "Knokke Film Scanner (VID: 0x" << std::hex << std::uppercase << std::setfill('0')
            << std::setw(4) << desc.idVendor << ", PID: 0x" << std::setw(4) << desc.idProduct
            << ")";
        return oss.str();
    }
    else
    {
        std::ostringstream oss;
        oss << "Knokke Film Scanner (VID: 0x" << std::hex << std::uppercase << std::setfill('0')
            << std::setw(4) << VENDOR_ID << ", PID: 0x" << std::setw(4) << PRODUCT_ID << ")";
        return oss.str();
    }
}

Knokke::Error Knokke::enterBootloader()
{
    if (!m_connected)
    {
        return Error::DEVICE_NOT_CONNECTED;
    }

    uint8_t data[1] = {0x01}; // Enter bootloader mode
    return performControlTransfer(UVC_REQUEST_TYPE_CLASS_OUT,
                                  UVC_SET_CUR,
                                  UVC_UPDATE_CONTROL,
                                  UVC_EXTENSION_UNIT,
                                  data,
                                  sizeof(data));
}

Knokke::Error Knokke::performControlTransfer(uint8_t  requestType,
                                             uint8_t  request,
                                             uint16_t value,
                                             uint16_t index,
                                             uint8_t *data,
                                             uint16_t length,
                                             int      timeout)
{
    int result = libusb_control_transfer(
        m_deviceHandle, requestType, request, value, index, data, length, timeout);

    if (result < 0)
    {
        handleError(Error::CONTROL_TRANSFER_FAILED,
                    "Control transfer failed: " + std::string(libusb_error_name(result)));
        return Error::CONTROL_TRANSFER_FAILED;
    }

    return Error::SUCCESS;
}

Knokke::Error Knokke::sendProbeCommit()
{
    // UVC probe control data (34 bytes as per UVC 1.1 spec)
    unsigned char probeData[34] = {
        0x00, 0x00, // bmHint: No fixed parameters
        0x01,       // bFormatIndex: Use 1st Video format index (YUY2)
        0x02,       // bFrameIndex: Use 2nd Video frame index (3840x12@400fps)
        0xa7, 0x61,
        0x00, 0x00, // dwFrameInterval: Desired frame interval in 100ns = (1/400.0)x10^7
        0x00, 0x00, // wKeyFrameRate: Key frame rate in key frame/video frame units
        0x00, 0x00, // wPFrameRate: PFrame rate in PFrame / key frame units
        0x00, 0x00, // wCompQuality: Compression quality control
        0x00, 0x00, // wCompWindowSize: Window size for average bit rate
        0x00, 0x00, // wDelay: Internal video streaming i/f latency in ms
        0x00, 0x68,
        0x01, 0x00, // dwMaxVideoFrameSize: Max video frame size in bytes = 3840 x 12 x 2
        0x00, 0x90,
        0x00, 0x00, // dwMaxPayloadTransferSize: No. of bytes device can rx in single payload: 36KB
        0x00, 0x60,
        0xE3, 0x16, // dwClockFrequency: Device Clock
        0x00,       // bmFramingInfo: Framing Information - Ignored for uncompressed format
        0x00,       // bPreferedVersion: Preferred payload format version
        0x00,       // bMinVersion: Minimum payload format version
        0x00        // bMaxVersion: Maximum payload format version
    };

    // Send probe control
    Error result = performControlTransfer(UVC_REQUEST_TYPE_CLASS_OUT,
                                          UVC_SET_CUR,
                                          UVC_VS_PROBE_CONTROL,
                                          UVC_STREAMING_INTERFACE,
                                          probeData,
                                          sizeof(probeData));
    if (result != Error::SUCCESS)
    {
        return result;
    }

    // Send commit control
    return performControlTransfer(UVC_REQUEST_TYPE_CLASS_OUT,
                                  UVC_SET_CUR,
                                  UVC_VS_COMMIT_CONTROL,
                                  UVC_STREAMING_INTERFACE,
                                  probeData,
                                  sizeof(probeData));
}

void Knokke::captureThreadFunction()
{
    const int            payloadSize = 32768;
    std::vector<uint8_t> payload(payloadSize);
    std::vector<uint8_t> frame;
    frame.reserve(FRAME_BYTES);

    const uint8_t UVC_HEADER_EOF = 1u << 1;

    while (m_threadRunning)
    {
        if (!m_streaming)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        frame.clear();
        bool gotEof     = false;
        int  safetyIter = 0;

        while (!gotEof && frame.size() < FRAME_BYTES && safetyIter < 1000 && m_threadRunning)
        {
            ++safetyIter;

            int transferred = 0;
            int result      = libusb_bulk_transfer(
                m_deviceHandle, BULK_EP_IN, payload.data(), payloadSize, &transferred, 200);

            if (result == LIBUSB_ERROR_TIMEOUT)
            {
                continue;
            }
            if (result < 0)
            {
                handleError(Error::USB_ERROR,
                            "Bulk transfer error: " + std::string(libusb_error_name(result)));
                break;
            }
            if (transferred <= 0)
            {
                continue;
            }

            // Parse UVC payload header
            if (transferred < 2)
                continue;

            const uint8_t *p         = payload.data();
            const uint8_t  headerLen = p[0];
            const uint8_t  headerBfh = p[1];

            if (headerLen > transferred)
                continue;

            const uint8_t *img      = p + headerLen;
            const int      imgBytes = transferred - headerLen;

            if (imgBytes > 0)
            {
                size_t spaceLeft = FRAME_BYTES - frame.size();
                size_t toCopy    = std::min(static_cast<size_t>(imgBytes), spaceLeft);
                frame.insert(frame.end(), img, img + toCopy);
            }

            if (headerBfh & UVC_HEADER_EOF)
            {
                gotEof = true;
            }
        }

        if (frame.size() >= FRAME_BYTES && m_frameCallback)
        {
            uint64_t frameNum = m_frameNumber.fetch_add(1);
            m_frameCallback(frame.data(), FRAME_BYTES, frameNum);
        }
    }
}

void Knokke::handleError(Error error, const std::string &message)
{
    if (m_errorCallback)
    {
        m_errorCallback(error, message);
    }
}

bool Knokke::findDevice()
{
    libusb_device **devices;
    ssize_t         deviceCount = libusb_get_device_list(m_context, &devices);

    if (deviceCount < 0)
    {
        handleError(Error::USB_ERROR,
                    "Failed to get device list: " + std::string(libusb_error_name(deviceCount)));
        return false;
    }

    for (ssize_t i = 0; i < deviceCount; ++i)
    {
        libusb_device           *device = devices[i];
        libusb_device_descriptor desc;

        int result = libusb_get_device_descriptor(device, &desc);
        if (result < 0)
        {
            continue;
        }

        if (desc.idVendor == VENDOR_ID && desc.idProduct == PRODUCT_ID)
        {
            int openResult = libusb_open(device, &m_deviceHandle);
            libusb_free_device_list(devices, 1);

            if (openResult < 0)
            {
                handleError(Error::DEVICE_OPEN_FAILED,
                            "Failed to open device: " + std::string(libusb_error_name(openResult)));
                return false;
            }

            return true;
        }
    }

    libusb_free_device_list(devices, 1);
    return false;
}

Knokke::Error Knokke::claimInterfaces()
{
    // Ensure configuration 1 is active
    int currentCfg = -1;
    libusb_get_configuration(m_deviceHandle, &currentCfg);
    if (currentCfg != 1)
    {
        int result = libusb_set_configuration(m_deviceHandle, 1);
        if (result < 0)
        {
            handleError(Error::USB_ERROR,
                        "Failed to set configuration: " + std::string(libusb_error_name(result)));
            return Error::USB_ERROR;
        }
    }

    // Find the streaming interface
    libusb_config_descriptor *cfg = nullptr;
    libusb_get_active_config_descriptor(libusb_get_device(m_deviceHandle), &cfg);

    int  streamIfNum = -1;
    int  streamAlt   = -1;
    bool epFound     = false;

    for (uint8_t i = 0; i < cfg->bNumInterfaces && !epFound; ++i)
    {
        const libusb_interface &iface = cfg->interface[i];
        for (int a = 0; a < iface.num_altsetting && !epFound; ++a)
        {
            const libusb_interface_descriptor &idesc = iface.altsetting[a];
            for (uint8_t e = 0; e < idesc.bNumEndpoints && !epFound; ++e)
            {
                const libusb_endpoint_descriptor &ep   = idesc.endpoint[e];
                uint8_t                           addr = ep.bEndpointAddress;
                uint8_t                           attr = ep.bmAttributes & 0x3;

                if (((addr & 0x8F) == BULK_EP_IN) && (attr == LIBUSB_TRANSFER_TYPE_BULK))
                {
                    streamIfNum = idesc.bInterfaceNumber;
                    streamAlt   = idesc.bAlternateSetting;
                    epFound     = true;
                }
            }
        }
    }

    if (!epFound)
    {
        handleError(Error::USB_ERROR,
                    "Could not locate BULK IN endpoint 0x" + std::to_string(BULK_EP_IN) +
                        " in active config");
        libusb_free_config_descriptor(cfg);
        return Error::USB_ERROR;
    }

    // Claim the streaming interface
    int result = libusb_claim_interface(m_deviceHandle, streamIfNum);
    if (result < 0)
    {
        handleError(Error::USB_ERROR,
                    "Failed to claim interface " + std::to_string(streamIfNum) + ": " +
                        std::string(libusb_error_name(result)));
        libusb_free_config_descriptor(cfg);
        return Error::USB_ERROR;
    }

    if (streamAlt > 0)
    {
        result = libusb_set_interface_alt_setting(m_deviceHandle, streamIfNum, streamAlt);
        if (result < 0)
        {
            handleError(Error::USB_ERROR,
                        "Failed to set alternate setting: " +
                            std::string(libusb_error_name(result)));
            libusb_free_config_descriptor(cfg);
            return Error::USB_ERROR;
        }
    }

    libusb_free_config_descriptor(cfg);
    return Error::SUCCESS;
}

void Knokke::releaseInterfaces()
{
    if (m_deviceHandle)
    {
        // Release all interfaces
        for (int i = 0; i < 8; ++i)
        { // Check first 8 interfaces
            libusb_release_interface(m_deviceHandle, i);
        }
    }
}
