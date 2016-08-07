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

#include "NotificationWindowManager.h"
#include <algorithm>

using namespace ukot::ie_web_notifications;

NotificationWindowManager::NotificationWindowManager(IMessageLoop* messageLoop, Settings& settings)
  : m_messageLoop(messageLoop)
  , m_settings(settings)
{
  m_requestSettingsShowing = [this]{
    m_messageLoop->dispatch([this]{
      if (!!onShowSettingsRequested) {
        onShowSettingsRequested();
      }
    });
  };
}

NotificationWindowManager::~NotificationWindowManager() {

}

void NotificationWindowManager::addNotification(const Notification& notification) {
  auto ii_notification = std::find_if(m_notifications.begin(), m_notifications.end(),
    [&notification](const std::shared_ptr<NotificationWithWindow>& value)->bool{
      return value->notification.origin == notification.origin
        && !value->notification.tag.empty()
        && !notification.tag.empty()
        && value->notification.tag == notification.tag;
    });
  if (m_notifications.end() != ii_notification) {
    if (!!(*ii_notification)->notification.aboutToBeDestroyed) {
      (*ii_notification)->notification.aboutToBeDestroyed(NotificationDestroyReason::replaced);
    }
    (*ii_notification)->notification = notification;
    if (!!(*ii_notification)->window) {
      (*ii_notification)->window->setNotificationBody(notification);
    }
    if (!!(*ii_notification)->notification.shown) {
      (*ii_notification)->notification.shown();
    }
  } else {
    m_notifications.push_back(std::make_shared<NotificationWithWindow>(notification));
    updateNotificationWindows();
  }
}

void NotificationWindowManager::closeNotification(const NotificationId& notificationId) {
  auto ii_notification = std::find_if(m_notifications.begin(), m_notifications.end(),
    [&notificationId](const std::shared_ptr<NotificationWithWindow>& value)->bool{
    return notificationId == value->notification.id;
  });
  if (m_notifications.end() == ii_notification) {
    return;
  }
  if (!!(*ii_notification)->window) {
    (*ii_notification)->window->destroy();
  } else {
    if (!!(*ii_notification)->notification.aboutToBeDestroyed) {
      (*ii_notification)->notification.aboutToBeDestroyed(NotificationDestroyReason::closed);
    }
    m_notifications.erase(ii_notification);
  }
}

void NotificationWindowManager::createNotificationWindow(const std::shared_ptr<NotificationWithWindow>& notification) {
  std::weak_ptr<NotificationWithWindow> notificationForClosure = notification;
  auto onWindowDestroyed = [this, notificationForClosure](bool clicked) {
    m_messageLoop->dispatch([this, notificationForClosure, clicked]{
      if (auto notification = notificationForClosure.lock()) {
        // 1. send response
        if (!!notification->notification.aboutToBeDestroyed) {
          notification->notification.aboutToBeDestroyed(clicked ? NotificationDestroyReason::clicked : NotificationDestroyReason::closed);
        }
        // 2. delete window
        auto ii_notification = std::find_if(m_notifications.begin(), m_notifications.end(),
          [&notification](const std::shared_ptr<NotificationWithWindow>& value)->bool{
          return value.get() == notification.get();
        });
        // we should do it at the end of the method, because the destroying of the window also
        // destroyes the context.
        m_notifications.erase(ii_notification);
        updateNotificationWindows();
      }
    });
  };
  notification->window = m_settings.notificationWindowFactory().create(notification->notification, onWindowDestroyed, m_requestSettingsShowing);
  if (!notification->window) {
    return;
  }
}

void NotificationWindowManager::updateNotificationWindows() {
  uint8_t shownWindows = 0;
  // if cannot create window mark it as error notification and send error and remove it after the current loop
  std::vector<NotificationId> errorNotifications;
  auto ii_notification = m_notifications.begin();
  for (; shownWindows < 3 && m_notifications.end() != ii_notification; ++ii_notification) {
    if (!(*ii_notification)->window) {
      createNotificationWindow(*ii_notification);
      if (!!(*ii_notification)->window) {
        if (!!(*ii_notification)->notification.shown) {
          (*ii_notification)->notification.shown();
        }
      } else {
        errorNotifications.push_back((*ii_notification)->notification.id);
      }
    }
    if (!!(*ii_notification)->window)
      (*ii_notification)->window->setSlot(shownWindows++);
  }
  auto ii_removed = std::remove_if(m_notifications.begin(), m_notifications.end(),
    [&](const std::shared_ptr<NotificationWithWindow>& notificationWithWnd)->bool{
    return std::find(errorNotifications.begin(), errorNotifications.end(), notificationWithWnd->notification.id) != errorNotifications.end();
  });
  for (; ii_removed != m_notifications.end(); ++ii_removed) {
    // TODO: send error to the client
  }
  m_notifications.erase(ii_removed, m_notifications.end());
}
