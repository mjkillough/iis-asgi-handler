#pragma once

#include <string>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Oleauto.h>


template<typename T, typename S = T>
class ScopedResourceTraits
{
public:
    static constexpr T invalid_value{ nullptr };
    static T Wrap(S v) { return v; }
    static void Release(T v) { static_assert(false, "Unknown resource type"); }
};


template<typename T, typename S = T>
class ScopedResource
{
public:
    ScopedResource() = default;
    explicit ScopedResource(S value) { Set(value); }
    ScopedResource(ScopedResource&) = delete;
    ScopedResource(ScopedResource&& other) { Set(other.Transfer()); }

    ~ScopedResource() { Release(); }

    void operator=(ScopedResource&) = delete;
    ScopedResource& operator=(ScopedResource&& other)
    {
        if (this != &other) {
            Set(other.Transfer());
            return *this;
        }
    }

    // Convenience for pointer-like Ts.
    T operator->() { return m_value; }

    bool IsValid() const { return m_value != traits::invalid_value; }
    T Get() const { return m_value; }
    void Set(S value) { m_value = traits::Wrap(value); }
    T* Receive() { return &m_value; }

    T Transfer()
    {
        auto tmp = m_value;
        m_value = traits::invalid_value;
        return tmp;
    }

    void Release()
    {
        if (IsValid()) {
            traits::Release(m_value);
            m_value = traits::invalid_value;
        }
    }

private:
    using traits = ScopedResourceTraits<T, S>;

    T m_value{ traits::invalid_value };
};


template<>
class ScopedResourceTraits<HANDLE>
{
public:
    static constexpr HANDLE invalid_value{ nullptr };
    static HANDLE Wrap(HANDLE h) { return h; }
    static void Release(HANDLE h) { ::CloseHandle(h); }
};
using ScopedHandle = ScopedResource<HANDLE>;


template<>
class ScopedResourceTraits<BSTR, PCWSTR>
{
public:
    static constexpr BSTR invalid_value{ nullptr };
    static BSTR Wrap(PCWSTR str) { return ::SysAllocString(str); }
    static void Release(BSTR b) { ::SysFreeString(b); }
};
using ScopedBstr = ScopedResource<BSTR, PCWSTR>;


template<typename T>
class ScopedResourceTraits<T*>
{
public:
    static constexpr T* invalid_value{ nullptr };
    static T* Wrap(T* str) { return v; }
    static void Release(T* v) { v->Release(); }
};
template<typename T>
using ScopedConfig = ScopedResource<T*>;
