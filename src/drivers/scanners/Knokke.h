#ifndef KNOKKE_H
#define KNOKKE_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <libusb-1.0/libusb.h>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

/**
 * @brief Knokke Film Scanner Interface Class
 *
 * This class provides a comprehensive interface to the Knokke Film Scanner device.
 * It supports parameter control (gain, backlight, exposure, motor speed), video streaming,
 * and frame capture with callback mechanisms. The class is designed to be POSIX compliant
 * and functional on macOS, Windows, and Linux.
 */
class Knokke
{
  public:
    // Device identifiers
    static constexpr uint16_t VENDOR_ID  = 0x34b4;
    static constexpr uint16_t PRODUCT_ID = 0x00c3;

    // UVC Control Request Types
    static constexpr uint8_t UVC_REQUEST_TYPE_CLASS     = 0xA1; // Device-to-host, Class, Interface
    static constexpr uint8_t UVC_REQUEST_TYPE_CLASS_OUT = 0x21; // Host-to-device, Class, Interface

    // UVC Control Requests
    static constexpr uint8_t UVC_GET_CUR  = 0x81;
    static constexpr uint8_t UVC_GET_MIN  = 0x82;
    static constexpr uint8_t UVC_GET_MAX  = 0x83;
    static constexpr uint8_t UVC_GET_RES  = 0x84;
    static constexpr uint8_t UVC_GET_LEN  = 0x85;
    static constexpr uint8_t UVC_GET_INFO = 0x86;
    static constexpr uint8_t UVC_GET_DEF  = 0x87;
    static constexpr uint8_t UVC_SET_CUR  = 0x01;

    // UVC Control Values
    static constexpr uint16_t UVC_EXPOSURE_CONTROL =
        0x0300; // Control 0x03, Unit 0x01 (Camera Terminal)
    static constexpr uint16_t UVC_GAIN_CONTROL =
        0x0100; // Control 0x01, Unit 0x01 (Camera Terminal)
    static constexpr uint16_t UVC_BACKLIGHT_CONTROL =
        0x0100; // Control 0x01, Unit 0x03 (Extension Unit)
    static constexpr uint16_t UVC_MOTOR_SPEED_CONTROL =
        0x0200; // Control 0x02, Unit 0x03 (Extension Unit)
    static constexpr uint16_t UVC_UPDATE_CONTROL =
        0x0300; // Control 0x03, Unit 0x03 (Extension Unit)

    // UVC Control Indexes
    static constexpr uint16_t UVC_CAMERA_TERMINAL = 0x0100; // Unit 0x01 (Camera Terminal)
    static constexpr uint16_t UVC_EXTENSION_UNIT  = 0x0300; // Unit 0x03 (Extension Unit)

    // Video Streaming Control
    static constexpr uint16_t UVC_VS_PROBE_CONTROL   = 0x0100; // Probe control
    static constexpr uint16_t UVC_VS_COMMIT_CONTROL  = 0x0200; // Commit control
    static constexpr uint8_t UVC_STREAMING_INTERFACE = 0x01; // Interface 0x01 (Streaming Interface)

    // USB Endpoints
    static constexpr uint8_t BULK_EP_IN = 0x83; // CX3_EP_BULK_VIDEO

    // Frame parameters
    static constexpr int FRAME_WIDTH  = 3840;
    static constexpr int FRAME_HEIGHT = 12;
    static constexpr int FRAME_BYTES  = FRAME_WIDTH * FRAME_HEIGHT * 2; // RAW16 format
    static constexpr int FRAME_RATE   = 400;                            // fps

    // Error codes
    enum class Error
    {
        SUCCESS = 0,
        DEVICE_NOT_FOUND,
        DEVICE_OPEN_FAILED,
        DEVICE_NOT_CONNECTED,
        CONTROL_TRANSFER_FAILED,
        STREAMING_NOT_STARTED,
        STREAMING_ALREADY_STARTED,
        THREAD_CREATION_FAILED,
        INVALID_PARAMETER,
        USB_ERROR,
        UNKNOWN_ERROR
    };

    // Callback function types
    using FrameCallback =
        std::function<void(const uint8_t *frameData, size_t frameSize, uint64_t frameNumber)>;
    using ErrorCallback = std::function<void(Error error, const std::string &message)>;

    // Parameter structures
    struct BacklightParams
    {
        uint16_t red   = 0; // 0-65535
        uint16_t green = 0; // 0-65535
        uint16_t blue  = 0; // 0-65535
    };

    struct ScannerParams
    {
        uint32_t        exposure_time = 100; // microseconds
        uint16_t        gain          = 0;   // gain_db * 100 (0.01 dB units)
        BacklightParams backlight;
        int32_t         motor_speed      = 0; // steps/s
        bool            enter_bootloader = false;
    };

    /**
     * @brief Constructor
     */
    Knokke();

    /**
     * @brief Destructor
     */
    ~Knokke();

    /**
     * @brief Initialize the scanner interface
     * @return Error code indicating success or failure
     */
    Error initialize();

    /**
     * @brief Connect to the scanner device
     * @return Error code indicating success or failure
     */
    Error connect();

    /**
     * @brief Disconnect from the scanner device
     */
    void disconnect();

    /**
     * @brief Check if the scanner is connected
     * @return true if connected, false otherwise
     */
    bool isConnected() const;

    // Parameter control methods

    /**
     * @brief Get current exposure time
     * @param exposureTime Output parameter for exposure time in microseconds
     * @return Error code indicating success or failure
     */
    Error getExposureTime(uint32_t &exposureTime);

    /**
     * @brief Set exposure time
     * @param exposureTime Exposure time in microseconds
     * @return Error code indicating success or failure
     */
    Error setExposureTime(uint32_t exposureTime);

    /**
     * @brief Get current gain
     * @param gain Output parameter for gain (gain_db * 100)
     * @return Error code indicating success or failure
     */
    Error getGain(uint16_t &gain);

    /**
     * @brief Set gain
     * @param gain Gain value (gain_db * 100, e.g., 1200 for 12.00 dB)
     * @return Error code indicating success or failure
     */
    Error setGain(uint16_t gain);

    /**
     * @brief Get current backlight parameters
     * @param backlight Output parameter for backlight values
     * @return Error code indicating success or failure
     */
    Error getBacklight(BacklightParams &backlight);

    /**
     * @brief Set backlight parameters
     * @param backlight Backlight parameters (RGB values 0-65535)
     * @return Error code indicating success or failure
     */
    Error setBacklight(const BacklightParams &backlight);

    /**
     * @brief Set individual backlight channel
     * @param channel 'r', 'g', or 'b'
     * @param value Brightness value (0-65535)
     * @return Error code indicating success or failure
     */
    Error setBacklightChannel(char channel, uint16_t value);

    /**
     * @brief Get current motor speed
     * @param speed Output parameter for motor speed in steps/s
     * @return Error code indicating success or failure
     */
    Error getMotorSpeed(int32_t &speed);

    /**
     * @brief Set motor speed
     * @param speed Motor speed in steps/s (can be negative for reverse)
     * @return Error code indicating success or failure
     */
    Error setMotorSpeed(int32_t speed);

    /**
     * @brief Get all scanner parameters
     * @param params Output parameter for all scanner parameters
     * @return Error code indicating success or failure
     */
    Error getParameters(ScannerParams &params);

    /**
     * @brief Set all scanner parameters
     * @param params Scanner parameters to set
     * @return Error code indicating success or failure
     */
    Error setParameters(const ScannerParams &params);

    // Video streaming methods

    /**
     * @brief Start video streaming
     * @return Error code indicating success or failure
     */
    Error startStreaming();

    /**
     * @brief Stop video streaming
     * @return Error code indicating success or failure
     */
    Error stopStreaming();

    /**
     * @brief Check if streaming is active
     * @return true if streaming, false otherwise
     */
    bool isStreaming() const;

    /**
     * @brief Set frame callback function
     * @param callback Function to call when a new frame is received
     */
    void setFrameCallback(FrameCallback callback);

    /**
     * @brief Set error callback function
     * @param callback Function to call when an error occurs
     */
    void setErrorCallback(ErrorCallback callback);

    /**
     * @brief Capture a single frame
     * @param frameData Output buffer for frame data
     * @param frameSize Size of the frame buffer
     * @param timeoutMs Timeout in milliseconds
     * @return Error code indicating success or failure
     */
    Error captureFrame(uint8_t *frameData, size_t frameSize, int timeoutMs = 5000);

    /**
     * @brief Get the latest frame from streaming
     * @param frameData Output buffer for frame data
     * @param frameSize Size of the frame buffer
     * @return Error code indicating success or failure
     */
    Error getLatestFrame(uint8_t *frameData, size_t frameSize);

    /**
     * @brief Capture multiple frames
     * @param numFrames Number of frames to capture
     * @param frameCallback Callback function for each frame
     * @return Error code indicating success or failure
     */
    Error captureFrames(int numFrames, FrameCallback frameCallback);

    // Utility methods

    /**
     * @brief Get error message for error code
     * @param error Error code
     * @return Human-readable error message
     */
    static std::string getErrorMessage(Error error);

    /**
     * @brief Get device information
     * @return Device information string
     */
    std::string getDeviceInfo() const;

    /**
     * @brief Enter bootloader mode (for firmware updates)
     * @return Error code indicating success or failure
     */
    Error enterBootloader();

  private:
    // Private member variables
    libusb_context       *m_context;
    libusb_device_handle *m_deviceHandle;
    std::atomic<bool>     m_connected;
    std::atomic<bool>     m_streaming;
    std::atomic<bool>     m_threadRunning;

    // Threading
    std::unique_ptr<std::thread> m_captureThread;
    std::mutex                   m_mutex;
    std::condition_variable      m_condition;

    // Callbacks
    FrameCallback m_frameCallback;
    ErrorCallback m_errorCallback;

    // Frame capture state
    std::vector<uint8_t>  m_frameBuffer;
    std::atomic<uint64_t> m_frameNumber;

    // Latest frame storage for streaming
    std::vector<uint8_t> m_latestFrame;
    std::mutex           m_latestFrameMutex;

    // Private methods
    Error performControlTransfer(uint8_t  requestType,
                                 uint8_t  request,
                                 uint16_t value,
                                 uint16_t index,
                                 uint8_t *data,
                                 uint16_t length,
                                 int      timeout = 1000);

    Error sendProbeCommit();
    void  captureThreadFunction();
    void  handleError(Error error, const std::string &message);
    bool  findDevice();
    Error claimInterfaces();
    void  releaseInterfaces();
};

#endif // KNOKKE_H
