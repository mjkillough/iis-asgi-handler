#pragma once

#include <hiredis.h>


class ScopedRedisReply
{
public:
    explicit ScopedRedisReply(redisReply *reply)
        : m_reply(nullptr)
    {
        Set(reply);
    }

    ScopedRedisReply(ScopedRedisReply&& other)
    {
        m_reply = other.m_reply;
    }

    ScopedRedisReply(const ScopedRedisReply&) = delete;


    ~ScopedRedisReply()
    {
        Close();
    }

    redisReply* operator->()
    {
        return m_reply;
    }

    void Set(redisReply *reply)
    {
        if (m_reply != reply) {
            Close();
            m_reply = reply;
        }
    }

    void Close()
    {
        if (m_reply != nullptr) {
            freeReplyObject(m_reply);
            m_reply = nullptr;
        }

    }
private:
    redisReply *m_reply;
};
