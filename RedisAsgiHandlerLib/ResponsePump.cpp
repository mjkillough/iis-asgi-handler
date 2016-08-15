#include <chrono>
#include <mutex>

#define WIN32_LEAN_AND_MEAN
#include <ppltasks.h>

#include "ResponsePump.h"
#include "ChannelLayer.h"
#include "Logger.h"


ResponsePump::ResponsePump(const Logger& logger, ChannelLayer& channels)
    : logger(logger), m_channels(channels), m_thread_stop(false)
{
}

ResponsePump::~ResponsePump()
{
    m_thread_stop = true;
    // TODO: Consider m_callbacks.clear() and calling .detach()?
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void ResponsePump::Start()
{
    // TODO: Assert we aren't called twice.
    m_thread = std::thread(&ResponsePump::ThreadMain, this);
}

void ResponsePump::AddChannel(const std::string& channel, const ResponseChannelCallback& callback)
{
    std::lock_guard<std::mutex> lock(m_callbacks_mutex);
    logger.debug() << "ResponsePump::AddChannel(" << channel << ", _)";
    m_callbacks[channel] = callback;
}

void ResponsePump::RemoveChannel(const std::string& channel)
{
    std::lock_guard<std::mutex> lock(m_callbacks_mutex);
    m_callbacks.erase(channel);
}

void ResponsePump::ThreadMain()
{
    using namespace std::literals::chrono_literals;

    logger.debug() << "ResponsePump::ThreadMain() starting";
    while (!m_thread_stop) {
        std::vector<std::string> channel_names;
        {
            std::lock_guard<std::mutex> lock(m_callbacks_mutex);
            channel_names.reserve(m_callbacks.size());
            for (const auto& it : m_callbacks) {
                channel_names.push_back(it.first);
            }
        }

        // These delay values have been chosen to match those in asgi_redis.
        // TODO: Think about whether these make sense for us. Perhaps we should
        //       have some way of being woken up whilst we're sleeping?
        //       A 10-50ms latency seems like a pretty big hit.
        auto delay = 50ms;
        if (!channel_names.empty()) {
            delay = 10ms;
            
            std::string channel, data;
            std::tie(channel, data) = m_channels.ReceiveMany(channel_names, false);
            if (!channel.empty()) {
                delay = 0ms;

                std::lock_guard<std::mutex> lock(m_callbacks_mutex);
                const auto it = m_callbacks.find(channel);
                // If we don't have a callback, do nothing. Assume the request
                // has since been closed.
                if (it != m_callbacks.end()) {
                    const auto callback = it->second;
                    m_callbacks.erase(it);
                    logger.debug() << "ResponsePump calling callback for channel: " << channel;
                    concurrency::create_task([data, callback]() {
                        callback(std::move(data));
                    });
                } else {
                    logger.debug() << "ResponsePump dropping reply as no callback for channel: " << channel;
                }
            }
        }

        // Don't yield if there's more to dispatch.
        if (delay != 0ms) {
            std::this_thread::sleep_for(delay);
        }
    }
    logger.debug() << "ResponsePump::ThreadMain() exiting";
}
