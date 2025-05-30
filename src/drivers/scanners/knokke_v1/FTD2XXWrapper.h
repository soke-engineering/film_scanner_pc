#ifndef FTD2XXWRAPPER_H
#define FTD2XXWRAPPER_H

#include <atomic>
#include <condition_variable>
#include <ftd2xx.h>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

class FTD2XXWrapper
{
  public:
    using DataCallback = std::function<void(size_t bytesAdded)>;

    FTD2XXWrapper();
    ~FTD2XXWrapper();

    bool openBySerial(const std::string &serial);
    void close();

    void registerCallback(DataCallback cb);

    // Retrieve next data chunk from the queue (thread-safe)
    bool popData(std::vector<uint8_t> &outData);

  private:
    void readLoop();

    FT_HANDLE         ftHandle;
    std::thread       readThread;
    std::atomic<bool> running;

    std::queue<std::vector<uint8_t>> dataQueue;
    std::mutex                       queueMutex;
    std::condition_variable          dataCondition;

    DataCallback callback;
    std::mutex   callbackMutex;
};

#endif // FTD2XXWRAPPER_H
