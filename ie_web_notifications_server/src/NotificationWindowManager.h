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
#include "NotificationWindow.h"
#include <vector>
#include <memory>
#include "IMessageLoop.h"
#include "Settings.h"

namespace ukot { namespace ie_web_notifications {
  struct NotificationWithWindow {
    explicit NotificationWithWindow(const Notification& notification)
      : notification(notification)
    {}
    Notification notification;
    std::unique_ptr<INotificationWindow> window;
  };


  class NotificationWindowManager {
  public:
    explicit NotificationWindowManager(IMessageLoop* messageLoop, Settings& settings);
    ~NotificationWindowManager();
    void addNotification(const Notification& notification);
    void closeNotification(const NotificationId& notificationId);
    // TODO:
    // When pipe is disconnected
    //void removeNotifications(const std::function<bool(const NotificationId&)>& filter);
    size_t size() const {
      return m_notifications.size();
    }
    bool empty() const {
      return m_notifications.empty();
    }
    std::function<void()> onShowSettingsRequested;
  private:
    void createNotificationWindow(const std::shared_ptr<NotificationWithWindow>& notificationWindow);
    void updateNotificationWindows();
  private:
    IMessageLoop* m_messageLoop = nullptr;
    Settings& m_settings;
    std::vector<std::shared_ptr<NotificationWithWindow>> m_notifications;
    std::function<void()> m_requestSettingsShowing;
  };
}}