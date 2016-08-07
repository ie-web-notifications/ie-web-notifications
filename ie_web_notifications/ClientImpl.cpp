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

#include "stdafx.h"
#include "ClientImpl.h"
#include <sergz.utils/utils.h>
#include <mutex>
#include <algorithm>
#include <ie_web_notifications.server.client/StartServer.h>
#include <ie_web_notifications.utils/convertTo.h>

using namespace ukot::ie_web_notifications;

using ukot::ie_web_notifications::util::convertToString;

namespace {
  const wchar_t* g_kPipeName = L"\\\\.\\pipe\\ie_web_notification.server.pipe";
}

ClientImpl::ClientImpl()
  : m_state{State::connecting}
  , m_exitThread{false}
  , m_notificationIdGenerator{1}
{
  registerChildMsgWindowClass();
  m_queueEvent = CreateEvent(/*sec attr*/nullptr, /*manual reset*/ FALSE,
    /*initial state*/FALSE, /* name */ nullptr);

  m_downloadQueueEvent = CreateEvent(/*sec attr*/nullptr, /*manual reset*/ FALSE,
    /*initial state*/FALSE, /* name */ nullptr);

  m_thread = std::thread{ [this]{
    threadFunc();
  }};

  m_downloadThread = std::thread{ [this]{
    downloadThreadFunc();
  } };
}

ClientImpl::~ClientImpl() {
  m_exitThread = true;
  if (0 == SetEvent(m_queueEvent.handle())) {
    // almost useless because the object is going to be destroyed
    DWORD lastError = GetLastError();
  }
  if (0 == SetEvent(m_downloadQueueEvent.handle())) {
    DWORD lastError = GetLastError();
  }
  if (m_downloadThread.joinable()) {
    m_downloadThread.join();
  }
  if (m_thread.joinable()){
    m_thread.join();
  }
}

NotificationId ClientImpl::notifyAsync(const ClientNotification& value) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  m_queue.emplace_back(value);
  auto& notification = *m_queue.rbegin();
  notification.id = ++m_notificationIdGenerator;
  auto ii = notification.icon.begin();
  std::wstring::const_iterator ii_end = notification.icon.end();
  utils::leftTrim(ii, ii_end);
  notification.iconDownloadStatus = [ii, ii_end]()-> IconDownloadStatus {
    if (ii == ii_end) {
      return IconDownloadStatus::none;
    }
    auto ii_begin = ii;
    if (utils::beginsWith_ci(ii_begin, ii_end, L"data:")) {
      return IconDownloadStatus::dataUrl;
    }
    return IconDownloadStatus::needToDownload;
  }();
  if (notification.iconDownloadStatus == IconDownloadStatus::needToDownload) {
    if (0 == SetEvent(m_downloadQueueEvent.handle())) {
      DWORD lastError = GetLastError();
    }
  } else{
    if (notification.iconDownloadStatus == IconDownloadStatus::dataUrl) {
      notification.iconFilePath.assign(ii, ii_end);
    }
    if (0 == SetEvent(m_queueEvent.handle())) {
      DWORD lastError = GetLastError();
    }
  }
  return m_queue.rbegin()->id;
}

void ClientImpl::closeNotificationAsync(NotificationId notificationId) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  m_closeNotifications.emplace_back(notificationId);
  if (0 == SetEvent(m_queueEvent.handle())) {
    DWORD lastError = GetLastError();
  }
}

struct ClientImpl::GetPermission {
  GetPermission() : id(0)
  {}

  uint64_t id;
  std::string origin;
  std::function<void(Permission)> callback;
};

Permission ClientImpl::getPermission(const std::wstring& origin) {
  Permission permission = Permission::default;
  utils::EventWithSetter eventWithSetter;
  {
    auto setter = eventWithSetter.getSetter();
    auto getPermissionObj = std::make_unique<GetPermission>();
    getPermissionObj->origin = convertToString(origin);
    getPermissionObj->callback = [setter, &permission, this](Permission value) {
      std::lock_guard<std::recursive_mutex> lock(m_mutex);
      permission = value;
      setter->set();
    };
    {
      std::lock_guard<std::recursive_mutex> lock(m_mutex);
      m_getPermissions.emplace_back(move(getPermissionObj));
      if (0 == SetEvent(m_queueEvent.handle())) {
        // almost useless because the object is going to be destroyed
        DWORD lastError = GetLastError();
      }
    }
  }
  eventWithSetter.wait(std::chrono::seconds(5));
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  return permission;
}

void ClientImpl::setPermissionAsync(const std::wstring& origin, Permission value) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  m_setPermissions.emplace_back(SetPermission{convertToString(origin), value});
  if (0 == SetEvent(m_queueEvent.handle())) {
    // almost useless because the object is going to be destroyed
    DWORD lastError = GetLastError();
  }
}

void ClientImpl::threadFunc() {
  while (!m_exitThread) {
    switch (m_state) {
    case State::connecting:
      establishConnection();
      // if cannot establish connection try again but be polite
      if (State::connecting == m_state) {
        std::this_thread::yield();
      }
      break;
    case State::connected:
      process();
      break;
    default:
      ;
    }
  }
}

void ClientImpl::downloadThreadFunc() {
  do {
    DWORD rc = WaitForSingleObject(m_downloadQueueEvent.handle(), INFINITE);
    switch (rc) {
    case WAIT_OBJECT_0:
      processDownloadQueue();
      break;
    default:
      ;
    };
  } while (!m_exitThread);
}

void ClientImpl::establishConnection() {
  if (!startProcess()) {
    return;
  }
  if (!connectToPipe()) {
    return;
  }
  m_state = State::connected;
}

bool ClientImpl::connectToPipe() {
  ukot::utils::Pipe::Params params;
  params.name = g_kPipeName;
  params.dataRead = [this](const void* data, size_t dataLength){
    onDataRead(data, dataLength);
  };
  params.disconnected = [this]{
    m_state = State::connecting;
  };
  m_pipe = ukot::utils::Pipe::connectTo(params);
  if (!m_pipe) {
    return false;
  }
  m_state = State::connected;
  m_pipe->readAsync();
  return true;
}

void ClientImpl::process() {
  do {
    DWORD rc = WaitForSingleObjectEx(m_queueEvent.handle(), INFINITE, TRUE);
    switch (rc) {
    case WAIT_IO_COMPLETION:
      break;
    case WAIT_OBJECT_0:
      processQueue();
      processGetPermissionsQueue();
      processSetPermissionsQueue();
      processCloseNotifictionsQueue();
      break;
    default:
      ;
    };
    if (m_state != State::connected) {
      std::lock_guard<std::recursive_mutex> lock(m_mutex);
      while (!m_queue.empty()) {
        auto failedResponse = NotificationErrorPM::Create(m_queue.begin()->id);
        onDataRead(failedResponse.data(), failedResponse.size());
      }
      while (!m_getPermissions.empty()) {
        auto failedResponse = GetPermissionResponsePM::Create((*m_getPermissions.begin())->id, Permission::denied);
        onDataRead(failedResponse.data(), failedResponse.size());
      }
      m_setPermissions.clear();
      m_closeNotifications.clear();
      m_pipe.reset();
      m_notificationIdGenerator = 1;
    }
  } while (!m_exitThread && State::connected == m_state);
}

void ClientImpl::processGetPermissionsQueue() {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  std::vector<std::unique_ptr<GetPermission>>::iterator ii_getPermission;
  // find first not processed yet
  ii_getPermission = std::find_if(begin(m_getPermissions), end(m_getPermissions),
    [](const std::unique_ptr<GetPermission>& value)->bool
  {
    return 0 == value->id;
  });
  if (m_getPermissions.end() == ii_getPermission) {
    return;
  }
  (*ii_getPermission)->id = ++m_notificationIdGenerator;
  auto isWritten = m_pipe->writeAsync(GetPermissionPM::Create((*ii_getPermission)->id, (*ii_getPermission)->origin));
  if (!isWritten) {
    auto failedResponse = GetPermissionResponsePM::Create((*ii_getPermission)->id, Permission::denied);
    onDataRead(failedResponse.data(), failedResponse.size());
  }
}

void ClientImpl::processSetPermissionsQueue() {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  for (const auto& setPermission : m_setPermissions) {
    m_pipe->writeAsync(SetPermissionPM::Create(setPermission.origin, setPermission.permission));
  }
  m_setPermissions.clear();
}

void ClientImpl::processCloseNotifictionsQueue() {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  for (const auto& closeNotificationId : m_closeNotifications) {
    m_pipe->writeAsync(CloseNotificationPM::Create(closeNotificationId));
  }
  m_closeNotifications.clear();
}

void ClientImpl::processDownloadQueue() {
  auto getNextToDownload = [this]()->std::wstring {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    auto it = std::find_if(begin(m_queue), end(m_queue), [](const ClientNotification& notification)->bool{
      return notification.iconDownloadStatus == IconDownloadStatus::needToDownload;
    });
    if (m_queue.end() == it) {
      return L"";
    }
    return it->icon;
  };
  std::wstring iconUrl;
  while (!(iconUrl = getNextToDownload()).empty()) {
    std::wstring fileName(1024, L'\0');
    // URLDownloadToCacheFile seems very dangerous but it's a price for easiness.
    // - can leak some sensitive data
    // - can exploit volnurabilities in nother protocols (registered globally by 3rd party apps).
    // - what about file:///?
    HRESULT hr = URLDownloadToCacheFileW(/*outer*/nullptr, m_queue.rbegin()->icon.c_str(), &fileName[0], fileName.size(), /*reserved*/0, nullptr);
    {
      // if there are several pending notifications with the same icon then download it only once.
      // To achieve it mark them all here.
      std::lock_guard<std::recursive_mutex> lock(m_mutex);
      for (auto& clientNotification : m_queue) {
        if (iconUrl != clientNotification.icon) {
          continue;
        }
        clientNotification.iconDownloadStatus = IconDownloadStatus::downloaded;
        if (S_OK == hr) {
          clientNotification.iconFilePath = fileName;
        }
      }
      if (0 == SetEvent(m_queueEvent.handle())) {
        DWORD lastError = GetLastError();
      }
    }
  }
}

void ClientImpl::processQueue() {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  for (auto& clientNotification : m_queue) {
    if (!clientNotification.isNeedToSend()) {
      continue;
    }
    auto title = convertToString(clientNotification.title);
    auto body = convertToString(clientNotification.body);
    auto origin = convertToString(clientNotification.origin);
    auto tag = convertToString(clientNotification.tag);
    auto iconFilePath = convertToString(clientNotification.iconFilePath);
    auto isWritten = m_pipe->writeAsync(ShowNotificationPM::Create(clientNotification.id, title, body, origin, tag, iconFilePath));
    if (!isWritten) {
      auto failedResponse = NotificationErrorPM::Create(clientNotification.id);
      onDataRead(failedResponse.data(), failedResponse.size());
    } else {
      clientNotification.isSent = true;
    }
  }
}

void ClientImpl::onDataRead(const void* data, size_t /*data_length*/) {
  auto msg = static_cast<const BaseProtocolMessage*>(data);
  if (nullptr == msg) {
    return;
  }
  switch (msg->commandType){
  case CommandType::notificationClosed:
    onNotificationClosed(*static_cast<const NotificationClosedPM*>(msg));
    break;
  case CommandType::notificationError:
    onNotificationError(*static_cast<const NotificationErrorPM*>(msg));
    break;
  case CommandType::notificationShown:
    onNotificationShown(*static_cast<const NotificationShownPM*>(msg));
    break;
  case CommandType::getPermissionResponse:
    onGetPermissionResponse(*static_cast<const GetPermissionResponsePM*>(msg));
    break;
  default:
    assert(false);
  }
}

void ClientImpl::onNotificationClosed(const NotificationClosedPM& msg) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  auto ii_clientNotification = std::find_if(begin(m_queue), end(m_queue), [&msg](const ClientNotification& value)->bool
  {
    return msg.clientNotificationId == value.id;
  });
  if (m_queue.end() == ii_clientNotification) {
    return;
  }
  if (!!ii_clientNotification->callback) {
    ii_clientNotification->callback(msg.closedBy);
  }
  m_queue.erase(ii_clientNotification);
}

void ClientImpl::onNotificationError(const NotificationErrorPM& msg) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  auto ii_clientNotification = std::find_if(begin(m_queue), end(m_queue), [&msg](const ClientNotification& value)->bool
  {
    return msg.clientNotificationId == value.id;
  });
  if (m_queue.end() == ii_clientNotification) {
    return;
  }
  if (!!ii_clientNotification->error) {
    ii_clientNotification->error();
  }
  m_queue.erase(ii_clientNotification);
}

void ClientImpl::onNotificationShown(const NotificationShownPM& msg) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  auto ii_clientNotification = std::find_if(begin(m_queue), end(m_queue), [&msg](const ClientNotification& value)->bool
  {
    return msg.clientNotificationId == value.id;
  });
  if (m_queue.end() == ii_clientNotification) {
    return;
  }
  if (!!ii_clientNotification->shown) {
    ii_clientNotification->shown();
  }
}

void ClientImpl::onGetPermissionResponse(const GetPermissionResponsePM& msg) {
  std::lock_guard<std::recursive_mutex> lock(m_mutex);
  auto ii_getPermission = std::find_if(begin(m_getPermissions), end(m_getPermissions),
    [&msg](const std::unique_ptr<GetPermission>& value)->bool
  {
    return msg.requestId == value->id;
  });
  if (m_getPermissions.end() == ii_getPermission) {
    return;
  }
  if (!!(*ii_getPermission)->callback) {
    (*ii_getPermission)->callback(msg.permission);
  }
  m_getPermissions.erase(ii_getPermission);
}

namespace {
  LRESULT CALLBACK ChildMsgWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
  }
}

void ClientImpl::registerChildMsgWindowClass() {
  WNDCLASSEX wcex{};

  wcex.cbSize = sizeof(wcex);
  wcex.style = 0;
  wcex.lpfnWndProc = ChildMsgWindowProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hInstance = ATL::_AtlBaseModule.GetModuleInstance();
  wcex.hIcon = nullptr;
  wcex.hCursor = nullptr;
  wcex.hbrBackground = nullptr;
  wcex.lpszMenuName = nullptr;
  wcex.lpszClassName = L"ie_web_notifications.ChildMsgWindowClass";
  wcex.hIconSm = nullptr;
  if (!RegisterClassEx(&wcex)) {
    DWORD err = GetLastError();
    throw std::runtime_error("Cannot register ChildMsgWindowClass");
  }
}