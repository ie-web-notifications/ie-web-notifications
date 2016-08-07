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
#include <cstdint>
#include <memory>

namespace ukot { namespace ie_web_notifications {
  struct NotificationBody;
  struct INotificationWindow {
    virtual ~INotificationWindow(){}
    virtual void destroy() = 0;
    virtual void setNotificationBody(const NotificationBody& notificationBody) = 0;
    virtual void setSlot(uint8_t slotNumber) = 0;
  };
  typedef std::unique_ptr<INotificationWindow> NotificationWindowPtr;
  struct INotificationWindowFactory {
    virtual ~INotificationWindowFactory(){}
    virtual NotificationWindowPtr create(const NotificationBody& notificationBody,
      const std::function<void(bool clicked)>& onWindowDestroyed,
      const std::function<void()>& onShowSettingsRequested) = 0;
  };
  typedef std::unique_ptr<INotificationWindowFactory> NotificationWindowFactoryPtr;
  NotificationWindowFactoryPtr createNativeWindowFactory();
  NotificationWindowFactoryPtr createToastWindowFactory();
  NotificationWindowFactoryPtr createD2DWindowFactory();
}}