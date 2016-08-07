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
#include <functional>
#include <Permission.h>
#include <NotificationDestroyReason.h>

namespace ukot { namespace ie_web_notifications {
  typedef uint64_t NotificationId;
  enum IconDownloadStatus: uint8_t {
    none /*also no need to download*/, needToDownload, dataUrl, downloaded
  };
  struct ClientNotification {
    bool isNeedToSend() const {
      return !isSent && iconDownloadStatus != IconDownloadStatus::needToDownload;
    }
    bool isSent = false;
    IconDownloadStatus iconDownloadStatus = IconDownloadStatus::none;
    NotificationId id = 0;
    std::wstring title;
    std::wstring body;
    std::wstring origin;
    std::wstring tag;
    std::wstring icon;
    std::wstring iconFilePath;
    std::function<void(NotificationDestroyReason arg)> callback;
    std::function<void()> error;
    std::function<void()> shown;
  };

  // we need hwnd to execute it in the proper context
  // we need Notification to call the callbacks on the proper Notification instance.
  // we need a global hash of <hwnd,X> which is protected by the lock.
  // subclass each window and remove hwnd from the global container when it's destroyed.
  // We need to keep each notification in the proper thread (put it into the container)
  // which is realeased in the proper thread.
  // Put into the global table <notification*, {hwnd}>.
  struct IClient {
    virtual ~IClient(){}
    // callback will be called from the bg thread, the caller is responsible to
    // prepare it, so the callback forwards the call to the proper thread.
    // As well as all captured variables will be deteled from the bg thread.
    virtual NotificationId notifyAsync(const ClientNotification& value) = 0;
    virtual void closeNotificationAsync(NotificationId notificationId) = 0;
    virtual Permission getPermission(const std::wstring& origin) = 0;
    virtual void setPermissionAsync(const std::wstring& origin, Permission value) = 0;
  };
  typedef std::shared_ptr<IClient> ClientPtr;
  
}}