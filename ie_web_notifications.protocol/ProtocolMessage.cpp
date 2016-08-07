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
#include "ProtocolMessage.h"

using namespace ukot::ie_web_notifications;

BufferData ShowNotificationPM::Create(uint64_t clientNotificationId, const std::string& title,
                                      const std::string& body, const std::string& origin,
                                      std::string& tag, const std::string& iconFilePath) {
  size_t protocolMessageLength = sizeof(ShowNotificationPM) + title.length() + body.length()
    + origin.length() + tag.length() + iconFilePath.length();
  std::vector<uint8_t> buffer;
  buffer.resize(protocolMessageLength);
  auto protocolMessage = reinterpret_cast<ShowNotificationPM*>(buffer.data());
  protocolMessage->commandType = CommandType::showNotification;
  protocolMessage->clientNotificationId =clientNotificationId;
  protocolMessage->titleLength = title.length();
  protocolMessage->bodyLength = body.length();
  protocolMessage->originLength = origin.length();
  protocolMessage->tagLength = tag.length();
  protocolMessage->iconFilePathLength = iconFilePath.length();
  memcpy(protocolMessage->data, title.c_str(), protocolMessage->titleLength);
  memcpy(protocolMessage->data + protocolMessage->titleLength, body.c_str(), protocolMessage->bodyLength);
  memcpy(protocolMessage->data + protocolMessage->titleLength + protocolMessage->bodyLength, origin.c_str(), protocolMessage->originLength);
  memcpy(protocolMessage->data + protocolMessage->titleLength + protocolMessage->bodyLength + protocolMessage->originLength, tag.c_str(), protocolMessage->tagLength);
  memcpy(protocolMessage->data + protocolMessage->titleLength + protocolMessage->bodyLength + protocolMessage->originLength + protocolMessage->tagLength, iconFilePath.c_str(), protocolMessage->iconFilePathLength);
  return buffer;
}

BufferData NotificationClosedPM::Create(uint64_t clientNotificationId, NotificationDestroyReason closedBy) {
  std::vector<uint8_t> buffer;
  buffer.resize(sizeof(NotificationClosedPM));
  auto responseMessage = reinterpret_cast<NotificationClosedPM*>(buffer.data());
  responseMessage->commandType = CommandType::notificationClosed;
  responseMessage->clientNotificationId = clientNotificationId;
  responseMessage->closedBy = closedBy;
  return buffer;
}

BufferData NotificationErrorPM::Create(uint64_t clientNotificationId) {
  std::vector<uint8_t> buffer;
  buffer.resize(sizeof(NotificationErrorPM));
  auto responseMessage = reinterpret_cast<NotificationErrorPM*>(buffer.data());
  responseMessage->commandType = CommandType::notificationError;
  responseMessage->clientNotificationId = clientNotificationId;
  return buffer;
}

BufferData NotificationShownPM::Create(uint64_t clientNotificationId) {
  std::vector<uint8_t> buffer;
  buffer.resize(sizeof(NotificationShownPM));
  auto responseMessage = reinterpret_cast<NotificationErrorPM*>(buffer.data());
  responseMessage->commandType = CommandType::notificationShown;
  responseMessage->clientNotificationId = clientNotificationId;
  return buffer;
}

BufferData GetPermissionPM::Create(uint64_t requestId, const std::string& origin) {
  size_t protocolMessageLength = sizeof(GetPermissionPM) + origin.length();
  std::vector<uint8_t> buffer;
  buffer.resize(protocolMessageLength);
  auto protocolMessage = reinterpret_cast<GetPermissionPM*>(buffer.data());
  protocolMessage->commandType = CommandType::getPermission;
  protocolMessage->requestId = requestId;
  protocolMessage->originLength = origin.length();
  memcpy(protocolMessage->data, origin.c_str(), protocolMessage->originLength);
  return buffer;
}

BufferData GetPermissionResponsePM::Create(uint64_t requestId, Permission permission) {
  std::vector<uint8_t> buffer;
  buffer.resize(sizeof(GetPermissionResponsePM));
  auto responseMessage = reinterpret_cast<GetPermissionResponsePM*>(buffer.data());
  responseMessage->commandType = CommandType::getPermissionResponse;
  responseMessage->requestId = requestId;
  responseMessage->permission = permission;
  return buffer;
}

BufferData SetPermissionPM::Create(const std::string& origin, Permission permission) {
  size_t protocolMessageLength = sizeof(SetPermissionPM) + origin.length();
  std::vector<uint8_t> buffer;
  buffer.resize(protocolMessageLength);
  auto protocolMessage = reinterpret_cast<SetPermissionPM*>(buffer.data());
  protocolMessage->commandType = CommandType::setPermission;
  protocolMessage->originLength = origin.length();
  protocolMessage->permission = permission;
  memcpy(protocolMessage->data, origin.c_str(), protocolMessage->originLength);
  return buffer;
}

BufferData CloseNotificationPM::Create(uint64_t clientNotificationId) {
  std::vector<uint8_t> buffer;
  buffer.resize(sizeof(CloseNotificationPM));
  auto responseMessage = reinterpret_cast<CloseNotificationPM*>(buffer.data());
  responseMessage->commandType = CommandType::closeNotification;
  responseMessage->clientNotificationId = clientNotificationId;
  return buffer;
}

BufferData ShowSettingsPM::Create() {
  std::vector<uint8_t> buffer;
  buffer.resize(sizeof(ShowSettingsPM));
  auto responseMessage = reinterpret_cast<ShowSettingsPM*>(buffer.data());
  responseMessage->commandType = CommandType::showSettings;
  return buffer;
}
