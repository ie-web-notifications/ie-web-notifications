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
#include <memory>
#include <condition_variable>
#include <chrono>
#include "Noncopyable.h"

namespace ukot { namespace utils {
  /// The aim of this class is to provide the means to wake up the waiting thread.
  /// It's a temporary solution until there is the correct future and promise
  /// implementation.
  /// Rationale:
  /// Let's consider the example:
  /// 1>{
  /// 2>  Event event;
  /// 3>  RunTask([&event]
  /// 4>  {
  /// 5>    // heavy task
  /// 6>    event.set();
  /// 7>  });
  /// 8>  waitResult = event.wait(timeout);
  /// 9>}
  /// If 'heavy task' takes more than `timeout` to execute then it will use
  /// dangling pointer to `event` because after `timeout` the caller's thread
  /// exits from the scope and destroyes `event`. So, we should pass something
  /// which is shared by the caller and by the task. In terms of this class
  /// it's `EventWithSetter::Data`. If 'heavy task' somehow exits from the scope
  /// (`return` or throws an exception) then the event won't be ever set and
  /// the caller will wait "forever" if timeout is "infinite" in the scope of
  /// the application. So, if `EventWithSetter::Setter` is destroyed the event is set
  /// but the EventWithSetter::Wait returns false.
  class EventWithSetter final: Noncopyable
  {
  public:
    class Setter;
  private:
    struct Data final {
      Data() : value(false), setCalled(false), setterCreated(false)
      {}
      // used as predicate value in std::condition_variable::wait_for
      bool value;
      std::mutex mutex;
      bool setCalled;
      std::condition_variable event;
      bool setterCreated;
    private:
      // uncopyable
      Data(const Data&);
      Data& operator=(const Data&);
    };
    // it's a shared_ptr because EventWithSetter can go away before setter will be
    // destroyed.
    typedef std::shared_ptr<Data> DataPtr;
  public:
    class Setter final: Noncopyable {
    public:
      Setter(const DataPtr& data);
      ~Setter();
      void set();
    private:
      DataPtr m_data;
    };
    EventWithSetter();
    /// Firstly one should obtain setter and only then it's allowed to call
    /// Wait. 
    std::shared_ptr<Setter> getSetter();
    bool wait(std::chrono::milliseconds timeout);
  private:
    DataPtr m_data;
  };
}}