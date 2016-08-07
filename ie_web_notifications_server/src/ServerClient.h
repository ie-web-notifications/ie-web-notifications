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
#include <functional>
#include <thread>
#include <sergz.utils/Noncopyable.h>
#include <sergz.utils/Pipe.h>
#include <deque>
#include <mutex>
#include <atomic>
#include <ProtocolMessage.h>

namespace ukot { namespace ie_web_notifications {

  class SynchronizedDeque final : utils::Noncopyable {
    typedef std::function<void()> TValue;
    struct Event final : utils::Noncopyable {
    public:
      Event() {
        m_eventImpl = CreateEvent(/*sec attr*/nullptr, /*manual reset*/ FALSE,
          /*initial state*/FALSE, /* name */ nullptr);
      }
      void notify() {
        SetEvent(m_eventImpl.handle());
      }
      void wait(std::unique_lock<std::recursive_mutex>& lock) {
        lock.unlock();
        DWORD rc = WaitForSingleObjectEx(m_eventImpl.handle(), INFINITE, TRUE);
        switch (rc) {
        case WAIT_IO_COMPLETION:
          break;
        case WAIT_OBJECT_0:
          break;
        default:
          ;
        };
        lock.lock();
      }
    private:
      ukot::utils::EventHandle m_eventImpl;
    };
  public:
    void push_back(const TValue& value) {
      std::lock_guard<std::recursive_mutex> lock(m_mutex);
      auto wasEmpty = m_deque.empty();
      m_deque.push_back(value);
      if (wasEmpty) {
        m_event.notify();
      }
    }
    void push_back(TValue&& value) {
      std::lock_guard<std::recursive_mutex> lock(m_mutex);
      auto wasEmpty = m_deque.empty();
      m_deque.push_back(std::move(value));
      if (wasEmpty) {
        m_event.notify();
      }
    }
    void push_front(const TValue& value) {
      std::lock_guard<std::recursive_mutex> lock(m_mutex);
      auto wasEmpty = m_deque.empty();
      m_deque.push_front(value);
      if (wasEmpty) {
        m_event.notify();
      }
    }
    void push_front(TValue&& value) {
      std::lock_guard<std::recursive_mutex> lock(m_mutex);
      auto wasEmpty = m_deque.empty();
      m_deque.push_front(std::move(value));
      if (wasEmpty) {
        m_event.notify();
      }
    }
    TValue pop_front() {
      std::unique_lock<std::recursive_mutex> lock(m_mutex);
      while (m_deque.empty()) {
        m_event.wait(lock);
      }
      TValue retValue = m_deque.front();
      m_deque.pop_front();
      return retValue;
    }
  private:
    std::recursive_mutex m_mutex;
    Event m_event;
    std::deque<TValue> m_deque;
  };

  class ServerClient final : utils::Noncopyable {
    enum class State : int {
      connecting, connected
    };
  public:
    ServerClient();
    ~ServerClient();
    void showSettings(const std::function<void()>& doneCallback);
  private:
    void threadFunc();
    void establishConnection();
    bool connectToPipe();
    void onDataRead(const void* data, size_t /*data_length*/);
  private:
    State m_state = State::connecting;
    std::thread m_thread;
    std::atomic<bool> m_exitThread = false;
    SynchronizedDeque m_calls;
    ukot::utils::Pipe::pointer m_pipe;
    uint64_t m_pipeMessageIDGenerator = 0;
  };
}}