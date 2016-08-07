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
#include <string>
#include <cstdint>
#include <functional>
#include <NotificationDestroyReason.h>
#include <ie_web_notifications.utils/NotificationBody.h>

namespace ukot { namespace ie_web_notifications {
  struct NotificationId {
    friend class Server;
    bool operator==(const NotificationId& other) const {
      return notificationId == other.notificationId && pipe == other.pipe;
    }
  protected:
    NotificationId(uint64_t notificationId, void* pipe)
      : notificationId(notificationId), pipe(pipe)
    {}
    uint64_t notificationId;
    void* pipe;
  };

  struct Notification : NotificationBody {
    Notification(const NotificationBody& notificationBody, const NotificationId id,
                 const std::string& origin, const std::string& tag)
      : NotificationBody(notificationBody), id(id), origin(origin), tag(tag)
    {}
    NotificationId id;
    std::string origin;
    std::string tag;
    // signals:
    std::function<void(NotificationDestroyReason closedBy)> aboutToBeDestroyed;
    std::function<void()> shown;
  };

}}