#include "FTD2XXWrapper.h"
#include <iostream>

FTD2XXWrapper::FTD2XXWrapper() : ftHandle(nullptr), running(false) {}

FTD2XXWrapper::~FTD2XXWrapper() { close(); }

bool FTD2XXWrapper::openBySerial(const std::string &serial)
{
    if (FT_OpenEx((void *)serial.c_str(), FT_OPEN_BY_SERIAL_NUMBER, &ftHandle) != FT_OK)
    {
        std::cerr << "Failed to open FTDI device with serial: " << serial << std::endl;
        return false;
    }

    FT_SetBaudRate(ftHandle, 115200);
    FT_SetDataCharacteristics(ftHandle, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
    FT_SetTimeouts(ftHandle, 500, 500);
    FT_SetUSBParameters(ftHandle, (DWORD)50e6, (DWORD)1e3);

    running    = true;
    readThread = std::thread(&FTD2XXWrapper::readLoop, this);

    return true;
}

void FTD2XXWrapper::close()
{
    running = false;

    if (readThread.joinable() == true)
    {
        readThread.join();
    }

    if (ftHandle != NULL)
    {
        FT_Close(ftHandle);
        ftHandle = nullptr;
    }

    // clear queue
    std::lock_guard<std::mutex>      lock(queueMutex);
    std::queue<std::vector<uint8_t>> empty;
    std::swap(dataQueue, empty);
}

void FTD2XXWrapper::registerCallback(DataCallback cb)
{
    std::lock_guard<std::mutex> lock(callbackMutex);
    callback = std::move(cb);
}

bool FTD2XXWrapper::popData(std::vector<uint8_t> &outData)
{
    std::lock_guard<std::mutex> lock(queueMutex);
    if (dataQueue.empty() == true)
    {
        return false;
    }
    outData = std::move(dataQueue.front());
    dataQueue.pop();
    return true;
}

void FTD2XXWrapper::readLoop()
{
    const DWORD bufferSize         = (DWORD)500e3;
    uint8_t     buffer[bufferSize] = {0};

    while (running == true)
    {
        DWORD     bytesRead = 0;
        FT_STATUS status    = FT_Read(ftHandle, buffer, bufferSize, &bytesRead);
        if ((status == FT_OK) && (bytesRead > 0))
        {
            std::vector<uint8_t> data(buffer, buffer + bytesRead);

            {
                std::lock_guard<std::mutex> lock(queueMutex);
                dataQueue.push(std::move(data));
            }

            // Notify
            std::lock_guard<std::mutex> lock(callbackMutex);
            if (callback)
            {
                callback(static_cast<size_t>(bytesRead));
            }

            dataCondition.notify_one();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
