/*

The MIT License (MIT)

Copyright (c) 2015 Martin Richter

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/

#ifndef TWPP_DETAIL_FILE_INTERNAL_HPP
#define TWPP_DETAIL_FILE_INTERNAL_HPP

#include "../twpp.hpp"

namespace Twpp {

struct AudioFileXfer {};
struct ImageFileXfer {};
typedef Memory IccProfileMemory;

namespace Detail {

typedef ReturnCode (TWPP_DETAIL_CALLSTYLE* DsmEntry)(
        Identity* origin,
        Identity* dest,
        DataGroup dg,
        Dat dat,
        Msg msg,
        void* data
);

typedef DsmEntry CallBackFunc;

typedef
#if defined(TWPP_DETAIL_OS_MAC)
void*
#else
UInt32
#endif
CallBackConstant;


TWPP_DETAIL_PACK_BEGIN
struct EntryPoint {
    constexpr EntryPoint() noexcept :
        m_size(sizeof(EntryPoint)), m_entry(nullptr), m_alloc(nullptr),
        m_free(nullptr), m_lock(nullptr), m_unlock(nullptr){}

    UInt32 m_size;
    DsmEntry m_entry;
    MemAlloc m_alloc;
    MemFree m_free;
    MemLock m_lock;
    MemUnlock m_unlock;
};

class CallBack {

public:
    constexpr CallBack(CallBackFunc func, CallBackConstant constant, Msg msg) noexcept :
        m_func(func), m_constant(constant), m_msg(msg){}

private:
    CallBackFunc m_func;
    CallBackConstant m_constant;
    Msg m_msg;

};

class CallBack2 {

public:
    constexpr CallBack2(CallBackFunc func, UIntPtr constant, Msg msg) noexcept :
        m_func(func), m_constant(constant), m_msg(msg){}

private:
    CallBackFunc m_func;
    UIntPtr m_constant;
    Msg m_msg;
};
TWPP_DETAIL_PACK_END

// stuff for handling DSM dll/so/framework
namespace DsmLibOs {

#if defined(TWPP_DETAIL_OS_WIN)
    typedef HMODULE Handle;
    static constexpr const Handle nullHandle = nullptr;

    static inline DsmEntry resolve(Handle h) noexcept{
        return reinterpret_cast<DsmEntry>(::GetProcAddress(h, "DSM_Entry"));
    }

    static inline Handle load(bool old) noexcept{
#   if defined(TWPP_DETAIL_OS_WIN32)
        if (old){
            auto h = ::LoadLibraryA("TWAIN_32.dll");
            if (!h){
                h = ::LoadLibraryA("TWAINDSM.dll");
            }

            return h;
        }
#   endif

        auto h = ::LoadLibraryA("TWAINDSM.dll");

#   if defined(TWPP_DETAIL_OS_WIN32)
        if (!h){
            h = ::LoadLibraryA("TWAIN_32.dll");
        }
#   else
        unused(old);
#   endif

        return h;
    }

    static inline void unload(Handle h) noexcept{
        ::FreeLibrary(h);
    }

#elif defined(TWPP_DETAIL_OS_MAC)
    typedef void* Handle;
    static constexpr const Handle nullHandle = nullptr;

    static inline DsmEntry resolve(Handle h) noexcept{
        return reinterpret_cast<DsmEntry>(::dlsym(h, "DSM_Entry"));
    }

    static inline Handle load(bool) noexcept{
        return ::dlopen("/System/Library/Frameworks/TWAIN.framework/TWAIN", RTLD_LAZY);
    }

    static inline void unload(Handle h) noexcept{
        ::dlclose(h);
    }

#elif defined(TWPP_DETAIL_OS_LINUX)
    typedef void* Handle;
    static constexpr const Handle nullHandle = nullptr;

    static inline DsmEntry resolve(Handle h) noexcept{
        return reinterpret_cast<DsmEntry>(::dlsym(h, "DSM_Entry"));
    }

    static inline Handle load(bool) noexcept{
        return ::dlopen("TWAINDSM.so", RTLD_LAZY);
    }

    static inline void unload(Handle h) noexcept{
        ::dlclose(h);
    }
#endif

}

/// Manages DSM dll/so/framework connection.
class DsmLib {

public:
    constexpr DsmLib() noexcept :
        m_handle(){}

    ~DsmLib(){
        unload();
    }

    DsmLib(const DsmLib&) = delete;
    DsmLib& operator=(const DsmLib&) = delete;

    DsmLib(DsmLib&& o) noexcept :
        m_handle(o.m_handle){

        o.m_handle = DsmLibOs::nullHandle;
    }

    DsmLib& operator=(DsmLib&& o) noexcept{
        if (&o != this){
            unload();

            m_handle = o.m_handle;
            o.m_handle = DsmLibOs::nullHandle;
        }

        return *this;
    }

    operator bool() const noexcept{
        return m_handle;
    }

    bool load(bool preferOld = false) noexcept{
        m_handle = DsmLibOs::load(preferOld);
        return m_handle;
    }

    void unload() noexcept{
        if (m_handle != DsmLibOs::nullHandle){
            DsmLibOs::unload(m_handle);
        }
    }

    DsmEntry resolve() const noexcept{
        return DsmLibOs::resolve(m_handle);
    }

private:
    DsmLibOs::Handle m_handle;

};

}

}

#endif // TWPP_DETAIL_FILE_INTERNAL_HPP