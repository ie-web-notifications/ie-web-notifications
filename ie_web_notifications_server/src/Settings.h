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
#include "PermissionDB.h"
#include "INotificationWindow.h"

namespace ukot { namespace ie_web_notifications {
  class Settings : utils::Noncopyable {
  public:
    Settings();
    ~Settings();
    void setToastNotificationsEnabled(bool enabled);
    bool isToastNotificationsEnabled() const {
      return m_isToastNotificationsEnabled;
    }
    std::function<void()> toastNotificationsEnabledChanged;

    bool canSupportToastNotificationsWindowFactory();

    PermissionDB& permissionDB() {
      return m_permissionDB;
    }
    INotificationWindowFactory& notificationWindowFactory();
  private:
    void load();
    void save();
  private:
    std::wstring m_filePath;
    bool m_isToastNotificationsEnabled = false;
    PermissionDB m_permissionDB;
    NotificationWindowFactoryPtr m_nativeNotificationWindowFactory;
    NotificationWindowFactoryPtr m_toastNotificationWindowFactory;
  };
}}