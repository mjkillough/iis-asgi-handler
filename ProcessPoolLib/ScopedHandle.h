#pragma once

#define LEAN_AND_MEAN
#include <Windows.h>


class ScopedHandle
{
public:
    ScopedHandle() = default;
    explicit ScopedHandle(HANDLE handle) { Set(handle); }
    ScopedHandle(ScopedHandle&& other) { Set(other.Transfer()); }
    ScopedHandle(ScopedHandle&) = delete;

    ~ScopedHandle() { Close(); }

    void operator=(ScopedHandle) = delete;

    ScopedHandle& operator=(ScopedHandle&& other)
    {
        if (this != &other) {
            Set(other.Transfer());
            return *this;
        }
    }

    ScopedHandle& operator=(HANDLE handle) {
        Set(handle);
        return *this;
    }

    bool IsValid() const { return m_handle != nullptr; }
    HANDLE Get() const { return m_handle; }
    operator HANDLE() const { return Get(); }

    void Set(HANDLE handle)
    {
        Close();
        if (handle != INVALID_HANDLE_VALUE) {
            m_handle = handle;
        }
    }

    HANDLE Transfer()
    {
        auto temp = m_handle;
        m_handle = nullptr;
        return m_handle;
    }

    void Close()
    {
        if (IsValid()) {
            ::CloseHandle(m_handle);
            m_handle = nullptr;
        }
    }

private:
    HANDLE m_handle{ nullptr };
};
