#pragma once

#include <thread>
#include <mutex>
#include <unordered_map>

#include "Logger.h"


class IChannelLayer;


class ResponsePump
{
public:
    ResponsePump(const Logger& logger, IChannelLayer& channels);
    ~ResponsePump();

    void Start();

    // TODO: Figure out exactly what this type should look like!
    using ResponseChannelCallback = std::function<void(std::string)>;

    virtual void AddChannel(const std::string& channel, const ResponseChannelCallback& callback);
    virtual void RemoveChannel(const std::string& channel);

private:
    void ThreadMain();


    const Logger& logger;
    IChannelLayer& m_channels;
    std::thread m_thread;
    bool m_thread_stop;
    std::unordered_map<std::string, ResponseChannelCallback> m_callbacks;
    std::mutex m_callbacks_mutex;
};

