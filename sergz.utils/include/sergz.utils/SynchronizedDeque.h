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
#include <queue>
#include <mutex>
#include <condition_variable>
#include "Noncopyable.h"

namespace ukot { namespace utils {
  template<typename T>
  class SynchronizedDeque: Noncopyable {
    /// The note about using std::lock_guard and std::unique_lock:
    /// - std::lock_guard is very light
    /// - std::unique_lock can be used in conjunction with std::condition_variable
  public:
    SynchronizedDeque(){};
    void push_back(const T& value) {
      std::lock_guard<std::mutex> lock(m_mutex);
      auto wasEmpty = m_deque.empty();
      m_deque.push_back(value);
      if (wasEmpty) {
        m_conditionVar.notify_one();
      }
    }
    void push_back(T&& value) {
      std::lock_guard<std::mutex> lock(m_mutex);
      auto wasEmpty = m_deque.empty();
      m_deque.push_back(std::move(value));
      if (wasEmpty) {
        m_conditionVar.notify_one();
      }
    }

    void push_front(const T& value) {
      std::lock_guard<std::mutex> lock(m_mutex);
      auto wasEmpty = m_deque.empty();
      m_deque.push_front(value);
      if (wasEmpty) {
        m_conditionVar.notify_one();
      }
    }
    void push_front(T&& value) {
      std::lock_guard<std::mutex> lock(m_mutex);
      auto wasEmpty = m_deque.empty();
      m_deque.push_front(std::move(value));
      if (wasEmpty) {
        m_conditionVar.notify_one();
      }
    }

    T pop_front() {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_conditionVar.wait(lock, [this]()->bool{
        return !m_deque.empty();
      });
      T retValue = m_deque.front();
      m_deque.pop_front();
      return retValue;
    }

    bool try_pop_front(const std::chrono::milliseconds& timeout, T& retValue) {
      std::unique_lock<std::mutex> lock(m_mutex);
      // false if the predicate still evaluates to false after the timeout expired, otherwise true.
      bool notExpired = m_conditionVar.wait_for(lock, timeout, [this]()->bool{
        return !m_deque.empty();
      });
      if (notExpired) {
        retValue = m_deque.front();
        m_deque.pop_front();
      }
      return notExpired;
    }

    void clear() {
      std::lock_guard<std::mutex> lock(m_mutex);
      m_deque.clear();
    }
  protected:
    // protected for tests.
    std::deque<T> m_deque;
    std::mutex m_mutex;
    std::condition_variable m_conditionVar;
  };
}}