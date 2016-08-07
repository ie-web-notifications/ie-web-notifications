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
#include "IClient.h"
#include <thread>
#include <atomic>
#include <mutex>
#include <deque>
#include <sergz.utils/Pipe.h>
#include <ProtocolMessage.h>

namespace ukot { namespace ie_web_notifications {
  class ClientImpl : public IClient {
    enum class State : int {
      connecting, connected
    };
  public:
    ClientImpl();
    ~ClientImpl();
    NotificationId notifyAsync(const ClientNotification& value) override;
    void closeNotificationAsync(NotificationId notificationId) override;
    Permission getPermission(const std::wstring& origin) override;
    void setPermissionAsync(const std::wstring& origin, Permission value) override;
  private:
    void threadFunc();
    void downloadThreadFunc();
    void establishConnection();
    void process();
    bool connectToPipe();
    void processQueue();
    void processGetPermissionsQueue();
    void processSetPermissionsQueue();
    void processCloseNotifictionsQueue();

    void processDownloadQueue();

    void onDataRead(const void* data, size_t data_length);

    void onNotificationClosed(const NotificationClosedPM& msg);
    void onNotificationError(const NotificationErrorPM& msg);
    void onNotificationShown(const NotificationShownPM& msg);
    void onGetPermissionResponse(const GetPermissionResponsePM& msg);

    void registerChildMsgWindowClass();
  private:
    std::thread m_thread;
    std::thread m_downloadThread;
    State m_state;
    std::atomic<bool> m_exitThread;
    ukot::utils::Pipe::pointer m_pipe;
    ukot::utils::EventHandle m_queueEvent;
    ukot::utils::EventHandle m_downloadQueueEvent;
    std::recursive_mutex m_mutex;
    std::vector<ClientNotification> m_queue;
    struct GetPermission;
    std::vector<std::unique_ptr<GetPermission>> m_getPermissions;
    struct SetPermission {
      std::string origin;
      Permission permission;
    };
    std::vector<SetPermission> m_setPermissions;
    std::vector<NotificationId> m_closeNotifications;
    uint64_t m_notificationIdGenerator;
  };
}}