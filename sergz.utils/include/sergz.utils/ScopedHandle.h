// IE Web Notifications
// Copyright (C) 2015-2016, Sergei Zabolotskikh. All rights reserved.
//
// This file is a part of IE Web Notifications project.
// The use and distribution terms for this software are covered by the
// BSD 3-Clause License (https://opensource.org/licenses/BSD-3-Clause)
// which can be found in the file LICENSE at the root of this distribution.
// By using this software in any fashion, you are agreeing to be bound by
// the terms of this license. You must not remove this notice, or
// any other, from this software.

#pragma once
#include <utility>
#include <Windows.h>
#include "Noncopyable.h"

namespace ukot { namespace utils {
  template<typename HandleTraits>
  class ScopedHandle final : Noncopyable
  {
    static_assert(std::is_trivial<typename HandleTraits::Handle>::value, "The Handle type should be trivial.");
  public:
    typedef HandleTraits Traits;
    explicit ScopedHandle(typename HandleTraits::Handle handle = HandleTraits::initialValue())
      : m_handle(handle)
    {
    }

    ~ScopedHandle()
    {
      close();
    }

    ScopedHandle& operator=(ScopedHandle&& src)
    {
      if (m_handle != src.m_handle)
      {
        std::swap(m_handle, src.m_handle);
        src.Close();
      }
      return *this;
    }

    ScopedHandle(ScopedHandle&& src) : m_handle(HandleTraits::initialValue())
    {
      std::swap(m_handle, src.m_handle);
    }

    ScopedHandle& operator=(typename HandleTraits::Handle h) {
      ScopedHandle closeCurrentAtTheEndOfMethod(m_handle);
      m_handle = h;
      return *this;
    }

    typename HandleTraits::Handle handle()
    {
      return m_handle;
    }

    bool operator!() const
    {
      return !isValid();
    }

    bool isValid() const
    {
      return HandleTraits::test(m_handle);
    }

    /// It returns nullptr if it holds the valid handle.
    typename HandleTraits::HandlePtr operator&()
    {
      if (isValid()) {
        return nullptr;
      }
      return &m_handle;
    }

    void close() {
      if (isValid()) {
        HandleTraits::close(m_handle);
        m_handle = HandleTraits::initialValue();
      }
    }
  private:
    typename HandleTraits::Handle m_handle;
  };

  // It's essential to specify the invalid value because for example very often invalid windows
  // handles are INVALID_HANDLE_VALUE and not zero.
  template<typename Handle, Handle InvalidValueT>
  struct BaseScopedHandleTraits
  {
    typedef Handle Handle;
    typedef Handle* HandlePtr;
    // required to be implemented
    //static bool Close(Handle h)

    static Handle initialValue()
    {
      return InvalidValueT;
    }
    static bool test(Handle value)
    {
      return InvalidValueT != value;
    }
  };

  template<HANDLE InvalidValue>
  struct WindowsHandleTraits : BaseScopedHandleTraits<HANDLE, InvalidValue>
  {
    static bool close(Handle h)
    {
      return 0 != CloseHandle(h);
    }
  };

  typedef ScopedHandle<WindowsHandleTraits<nullptr>> EventHandle;
  typedef ScopedHandle<WindowsHandleTraits<nullptr>> MutexHandle;
  typedef ScopedHandle<WindowsHandleTraits<nullptr>> IOCompletionPortHandle;
  typedef ScopedHandle<WindowsHandleTraits<nullptr>> ThreadHandle;
  typedef ScopedHandle<WindowsHandleTraits<nullptr>> ProcessHandle;
  // Zero (nullptr) invalid/initial value of the AccessToken is obtained experimentally.
  typedef ScopedHandle<WindowsHandleTraits<nullptr>> AccessToken;
  typedef ScopedHandle<WindowsHandleTraits<INVALID_HANDLE_VALUE>> FileHandle;
}}