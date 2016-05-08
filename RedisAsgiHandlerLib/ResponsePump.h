#pragma once

#include <thread>
#include <unordered_map>

#include "RedisChannelLayer.h"
#include "Logger.h"

class ResponsePump
{
public:
    ResponsePump(const Logger& logger);
    ~ResponsePump();

    void Start();

    // TODO: Figure out exactly what this type should look like!
    using ResponseChannelCallback = std::function<void(std::string)>;

    virtual void AddChannel(const std::string& channel, const ResponseChannelCallback& callback);
    virtual void RemoveChannel(const std::string& channel);

private:
    void ThreadMain();

    // This should probably be constructed by someone else and given to us.
    // (This will allow someone to configure it to speak to the right redis, etc.)
    RedisChannelLayer m_channels;

    const Logger& m_logger;
    std::thread m_thread;
    bool m_thread_stop;
    std::unordered_map<std::string, ResponseChannelCallback> m_callbacks;
    std::mutex m_callbacks_mutex;
};

