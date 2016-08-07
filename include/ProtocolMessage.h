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
#include <cstdint>
#include <vector>
#include "Permission.h"
#include "NotificationDestroyReason.h"

namespace ukot { namespace ie_web_notifications {
  enum class CommandType : uint8_t {
    showNotification, notificationClosed, notificationError, notificationShown, getPermission,
    setPermission, getPermissionResponse, closeNotification, showSettings
  };
#pragma pack(push, 1)
#pragma warning(push)
#pragma warning(disable:4200)
  struct BaseProtocolMessage {
    CommandType commandType;
  };

  typedef std::vector<uint8_t> BufferData;
  // client -> server
  struct ShowNotificationPM : BaseProtocolMessage {
    uint64_t clientNotificationId;
    uint16_t titleLength;
    uint16_t bodyLength;
    uint16_t originLength;
    uint16_t tagLength;
    uint16_t iconFilePathLength;
    char data[];
    static BufferData Create(uint64_t clientNotificationId, const std::string& title,
      const std::string& body, const std::string& origin, std::string& tag, const std::string& iconFilePath);
  };

  // server -> client
  struct NotificationClosedPM : BaseProtocolMessage {
    uint64_t clientNotificationId;
    // if the notification is closed using click then the value is not zero.
    NotificationDestroyReason closedBy;
    static BufferData Create(uint64_t clientNotificationId, NotificationDestroyReason closedBy);
  };

  // server->client
  struct NotificationErrorPM: BaseProtocolMessage {
    uint64_t clientNotificationId;
    static BufferData Create(uint64_t clientNotificationId);
  };

  // server->client
  struct NotificationShownPM : BaseProtocolMessage {
    uint64_t clientNotificationId;
    static BufferData Create(uint64_t clientNotificationId);
  };

  // client -> server
  struct GetPermissionPM : BaseProtocolMessage {
    uint64_t requestId;
    // if the notification is closed using click then the value is not zero.
    uint16_t originLength;
    char data[];
    static BufferData Create(uint64_t requestId, const std::string& origin);
  };

  // server -> client
  struct GetPermissionResponsePM : BaseProtocolMessage {
    uint64_t requestId;
    Permission permission;
    static BufferData Create(uint64_t requestId, Permission permission);
  };

  // server -> client
  struct SetPermissionPM : BaseProtocolMessage {
    Permission permission;
    uint16_t originLength;
    char data[];
    static BufferData Create(const std::string& origin, Permission permission);
  };

  // client -> server
  struct CloseNotificationPM : BaseProtocolMessage {
    uint64_t clientNotificationId;
    static BufferData Create(uint64_t clientNotificationId);
  };

  struct ShowSettingsPM : BaseProtocolMessage {
    static BufferData Create();
  };
#pragma warning(pop)
#pragma pack(pop)
}}