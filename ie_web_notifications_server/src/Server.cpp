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

#include "Server.h"
#include <ie_web_notifications.utils/convertTo.h>
#include <cassert>

using namespace ukot::ie_web_notifications;

namespace {
  enum CustomMessage {
    taskPosted
  };
}

Server::Server(const Params& params, Settings& settings)
  : m_settings(settings)
  , m_params(params)
  , m_listenOverlapped{}
  , m_notificationWindowManager(this, m_settings)
{
  m_connectEvent = CreateEvent(/*sec attr*/ nullptr, /*manual reset*/ FALSE,
    /*initial state*/ FALSE, /*name*/ nullptr);
  m_listenOverlapped.hEvent = m_eventHandles[0] = m_connectEvent.handle();
  m_noConnectionsTimer = std::make_unique<utils::ElapsedTimer>();
  createListeningPipe();
  listenPipeAsync();
  createMessageWindow();

  m_notificationWindowManager.onShowSettingsRequested = [this]{
    onShowSettings();
  };
}

Server::~Server() {
}

int Server::runMessageLoop() {
  MSG msg{};
  while (true/*!m_windows.empty()*/) {
    while (/*hasSomeMessages*/::PeekMessage(&msg, /*hwnd, any*/nullptr,
      0, 0, // filter min, filter max, zeros - return all available messages
      PM_REMOVE))
    {
      if (msg.message == WM_QUIT)
      {
        // Return the value that was passed with PostQuitMessage().
        //return msg.wParam;
      } else if (msg.hwnd == m_msgHWnd) {
        if (msg.message == CustomMessage::taskPosted) {
          std::lock_guard<std::recursive_mutex> lock(m_postedTasksMutex);
          while (!m_postedTasks.empty()) {
            auto task = *m_postedTasks.begin();
            m_postedTasks.pop_front();
            if (task)
              task();
          }
        }
      } else if (msg.hwnd == NULL) {
        // Thread messages have the hwnd member set to NULL. This
        // allows us to process thread messages here. If there is a
        // modal dialog open, however, then thread messages will not
        // be processed. Other techniques are required in this case.

        // TODO: Handle thread messages here.
      } else {
        // Translate virtual key messages into character messages.
        ::TranslateMessage(&msg);

        // Dispatch messages to the window procedures associated
        // with the thread in which this message loop is running.
        ::DispatchMessage(&msg);
      }
    } // End: while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    DWORD result = ::MsgWaitForMultipleObjectsEx(m_eventHandlesSize, m_eventHandles,
      static_cast<DWORD>(m_params.messageLoopWaitMsec.count()), QS_ALLINPUT, MWMO_ALERTABLE);
    if (m_eventHandlesSize - WAIT_OBJECT_0 > result) {
      // some event triggered
      // since now there is only one event, process connection
      onConnected();
      listenPipeAsync();
    } else if (WAIT_ABANDONED_0 == result) {
      // unexpected, event should exist
    } else if (WAIT_IO_COMPLETION == result) {
      // APC io completion queued, check the state of pipe array
    } else if (WAIT_TIMEOUT == result) {
      // nothing new, check if there is no client and it's already too late then exit
    } else if (WAIT_FAILED == result) {
      DWORD lastError = GetLastError();
    }
    if (nullptr != m_noConnectionsTimer) {
      if (m_noConnectionsTimer->hasExpired(m_params.exitTimeout)) {
        return 0;
      }
    }
  }
}

void Server::dispatch(const std::function<void()>& call) {
  if (std::this_thread::get_id() != m_threadID) {
    {
      std::lock_guard<std::recursive_mutex> lock(m_postedTasksMutex);
      m_postedTasks.push_back(call);
    }
    PostMessage(m_msgHWnd, CustomMessage::taskPosted, 0, 0);
  } else {
    call();
  }
}

void Server::createMessageWindow() {
  const std::wstring className = L"ie_web_notifications.message_hwnd";
  WNDCLASS wc;
  wc.style = 0;
  wc.lpfnWndProc = DefWindowProcW;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hInstance = ATL::_AtlBaseModule.GetModuleInstance();
  wc.hIcon = nullptr;
  wc.hCursor = nullptr;
  wc.hbrBackground = nullptr;
  wc.lpszMenuName = nullptr;
  wc.lpszClassName = className.c_str();
  ATOM atom = RegisterClass(&wc);
  if (!atom)
  {
    // Cannot register class for message only window
    return;
  }
  m_msgHWnd = CreateWindowW(className.c_str(),
    nullptr,      // window name
    0,            // style
    0, 0, 0, 0,   // geometry (x, y, w, h)
    HWND_MESSAGE, // parent
    nullptr,      // menu handle
    wc.hInstance,
    0);           // windows creation data.
  if (!m_msgHWnd)
  {
    // Cannot create message only window;
    return;
  }
}

void Server::listenPipeAsync() {
  if (nullptr == m_listeningPipe) {
    return;
  }
  bool connectNextPipe = false;
  do {
    connectNextPipe = false;
    BOOL rc = ConnectNamedPipe(m_listeningPipe->pipe->handle(), &m_listenOverlapped);
    DWORD lastError = GetLastError();
    if (0 == rc) {
      if (ERROR_IO_PENDING == lastError) {
        // pending
      } else if (ERROR_PIPE_CONNECTED == lastError) {
        onConnected();
        connectNextPipe = true;
      } else {
        //error
        std::string();
      }
    } else {
      // should not happen
      std::string();
    }
  } while (connectNextPipe);
}

void Server::onConnected() {
  m_noConnectionsTimer.reset();
  m_listeningPipe->pipe->readAsync();
  m_connectedPipes.emplace_back(std::move(m_listeningPipe));
  createListeningPipe();
}

void Server::createListeningPipe() {
  ukot::utils::Pipe::Params pipeParams;
  pipeParams.name = m_params.pipeName;
  auto pipe = m_listeningPipe = std::make_shared<utils::PipeHolder>();
  std::weak_ptr<utils::PipeHolder> weakPipe = pipe;
  pipeParams.disconnected = [this, weakPipe]{
    removePipeHolder(weakPipe.lock());
  };
  pipeParams.dataRead = [this, weakPipe](const void* data, size_t dataLength){
    if (auto pipe = weakPipe.lock()) {
      auto msg = static_cast<const BaseProtocolMessage*>(data);
      if (nullptr == msg) {
        return;
      }
      switch (msg->commandType){
      case CommandType::showNotification:
        onShowNotificationPM(*static_cast<const ShowNotificationPM*>(msg), pipe);
        break;
      case CommandType::closeNotification:
        onCloseNotificationPM(*static_cast<const CloseNotificationPM*>(msg), pipe);
        break;
      case CommandType::getPermission:
        onGetPermissionPM(*static_cast<const GetPermissionPM*>(msg), pipe);
        break;
      case CommandType::setPermission:
        onSetPermissionPM(*static_cast<const SetPermissionPM*>(msg), pipe);
        break;
      case CommandType::showSettings:
        onShowSettings();
        break;
      default:
        assert(false);
      }
    }
  };
  m_listeningPipe->pipe = ukot::utils::Pipe::createServer(pipeParams);
}

void Server::removePipeHolder(const ukot::utils::PipeHolderPtr& toRemoveValue) {
  auto ii = std::find_if(m_connectedPipes.begin(), m_connectedPipes.end(), [toRemoveValue](const utils::PipeHolderPtr& value)->bool{
    return value.get() == toRemoveValue.get();
  });
  if (m_connectedPipes.end() != ii) {
    m_connectedPipes.erase(ii);
  }
  if (m_connectedPipes.empty() && m_notificationWindowManager.empty() && !m_settingsWindow) {
    m_noConnectionsTimer = std::make_unique<ukot::utils::ElapsedTimer>();
  }
}

void Server::onShowNotificationPM(const ShowNotificationPM& msg, const ukot::utils::PipeHolderPtr& pipeHolderArg) {
  NotificationId notificationId = { msg.clientNotificationId, pipeHolderArg->pipe.get() };

  std::string origin{msg.data + msg.titleLength + msg.bodyLength, msg.originLength};
  std::string tag{msg.data + msg.titleLength + msg.bodyLength + msg.originLength, msg.tagLength};
  if (Permission::granted != m_settings.permissionDB().getPermission(origin)) {
    pipeHolderArg->pipe->writeAsync(NotificationErrorPM::Create(notificationId.notificationId));
    return;
  }

  using ukot::ie_web_notifications::util::convertToWString;
  NotificationBody notificationBody = {
    convertToWString(std::string{ msg.data, msg.titleLength }),
    convertToWString(std::string{ msg.data + msg.titleLength, msg.bodyLength }),
    convertToWString(std::string{ msg.data + msg.titleLength + msg.bodyLength +
      msg.originLength + msg.tagLength, msg.iconFilePathLength })
  };

  Notification notification{notificationBody, notificationId, origin, tag};
  std::weak_ptr<utils::PipeHolder> weakPipeHolder = pipeHolderArg;
  notification.aboutToBeDestroyed = [this, notificationId, weakPipeHolder](NotificationDestroyReason closedBy) {
    // 1. send response
    if (auto pipeHolder = weakPipeHolder.lock()) {
      pipeHolder->pipe->writeAsync(NotificationClosedPM::Create(notificationId.notificationId, closedBy));
    }
    // 2. start exit timer
    if (m_connectedPipes.empty() && !m_settingsWindow && m_notificationWindowManager.size() == 1) {
      m_noConnectionsTimer = std::make_unique<ukot::utils::ElapsedTimer>();
    }
    // windows is going to be deleted right after this function
  };

  notification.shown = [this, notificationId, weakPipeHolder]() {
    if (auto pipeHolder = weakPipeHolder.lock()) {
      pipeHolder->pipe->writeAsync(NotificationShownPM::Create(notificationId.notificationId));
    }
  };

  m_notificationWindowManager.addNotification(notification);
}

void Server::onCloseNotificationPM(const CloseNotificationPM& msg, const ukot::utils::PipeHolderPtr& pipeHolderArg) {
  NotificationId notificationId = { msg.clientNotificationId, pipeHolderArg->pipe.get() };
  m_notificationWindowManager.closeNotification(notificationId);
}

void Server::onGetPermissionPM(const GetPermissionPM& msg, const ukot::utils::PipeHolderPtr& pipeHolder) {
  std::string origin{msg.data, msg.originLength};
  Permission permission = m_settings.permissionDB().getPermission(origin);
  pipeHolder->pipe->writeAsync(GetPermissionResponsePM::Create(msg.requestId, permission));
}

void Server::onSetPermissionPM(const SetPermissionPM& msg, const ukot::utils::PipeHolderPtr& /*pipeHolder*/) {
  std::string origin{msg.data, msg.originLength};
  m_settings.permissionDB().setPermission(origin, msg.permission);
}

void Server::onShowSettings() {
  if (!!m_settingsWindow) {
    return;
  }
  m_settingsWindow = std::make_unique<SettingsBorderWindow>(m_settings);
  m_settingsWindow->Create(nullptr);
  if (nullptr == m_settingsWindow->operator HWND()) {
    return;
  }
  m_settingsWindow->ShowWindow(SW_SHOW);
  m_settingsWindow->UpdateWindow();
  m_settingsWindow->SetOnDestroyed([this]{
    if (m_connectedPipes.empty() && m_notificationWindowManager.size() == 0) {
      m_noConnectionsTimer = std::make_unique<ukot::utils::ElapsedTimer>();
    }
    m_settingsWindow.reset();
  });
}