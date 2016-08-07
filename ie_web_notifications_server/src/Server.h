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
#include <sergz.utils/Pipe.h>
#include <sergz.utils/ElapsedTimer.h>
#include <vector>
#include <chrono>
#include "NotificationWindowManager.h"
#include "SettingsWindow.h"
#include <ProtocolMessage.h>
#include "Settings.h"
#include <mutex>
#include "IMessageLoop.h"
#include <thread>
#include <deque>

namespace ukot { namespace ie_web_notifications {
  class Server final : public IMessageLoop {
  public:
    struct Params {
      std::wstring pipeName;
      std::chrono::milliseconds messageLoopWaitMsec;
      std::chrono::milliseconds exitTimeout;
    };
    Server(const Params& params, Settings& settings);
    ~Server();
    int runMessageLoop();
    void dispatch(const std::function<void()>& call) override;
  private:
    void createMessageWindow();
    void listenPipeAsync();
    void onConnected();
    void createListeningPipe();
    void removePipeHolder(const utils::PipeHolderPtr& toRemoveValue);
    void onShowNotificationPM(const ShowNotificationPM& msg, const utils::PipeHolderPtr& pipeHolder);
    void onCloseNotificationPM(const CloseNotificationPM& msg, const utils::PipeHolderPtr& pipeHolder);
    void onGetPermissionPM(const GetPermissionPM& msg, const utils::PipeHolderPtr& pipeHolder);
    void onSetPermissionPM(const SetPermissionPM& msg, const utils::PipeHolderPtr& pipeHolder);
    void onShowSettings();
  private:
    HWND m_msgHWnd = nullptr;
    Settings& m_settings;
    std::recursive_mutex m_postedTasksMutex;
    std::deque<std::function<void()>> m_postedTasks;
    std::thread::id m_threadID = std::this_thread::get_id();
    Params m_params;
    ukot::utils::EventHandle m_connectEvent;
    HANDLE m_eventHandles[1];
    std::vector<utils::PipeHolderPtr> m_connectedPipes;
    OVERLAPPED m_listenOverlapped;
    const uint16_t m_eventHandlesSize = sizeof(m_eventHandles) / sizeof(m_eventHandles[0]);
    utils::PipeHolderPtr m_listeningPipe;
    std::unique_ptr<ukot::utils::ElapsedTimer> m_noConnectionsTimer;
    NotificationWindowManager m_notificationWindowManager;
    std::unique_ptr<SettingsBorderWindow> m_settingsWindow;
  };
}}