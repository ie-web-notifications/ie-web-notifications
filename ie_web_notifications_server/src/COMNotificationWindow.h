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
#include "INotificationWindow.h"
#include <sergz.atl/atl.h>
#include "../ie_web_notifications.d2d/ie_web_notificationsd2d_i.h"

namespace ukot { namespace ie_web_notifications { namespace com_client {
  struct NotificationWindowImpl_Data {
    std::function<void(bool clicked)> onWindowDestroyed;
    std::function<void()> onShowSettingsRequested;
  };

  class ATL_NO_VTABLE NotificationWindowCallbackImpl :
    public ATL::CComObjectRootEx<ATL::CComMultiThreadModel>,
    public com::INotificationWindowCallback
  {
  public:
    explicit NotificationWindowCallbackImpl(const std::shared_ptr<com_client::NotificationWindowImpl_Data>& data)
      : m_data(data)
    {
    }
    DECLARE_NOT_AGGREGATABLE(NotificationWindowCallbackImpl)

    BEGIN_COM_MAP(NotificationWindowCallbackImpl)
      COM_INTERFACE_ENTRY(com::INotificationWindowCallback)
    END_COM_MAP()

    STDMETHOD(closed)(BOOL clicked) override;
    STDMETHOD(requestSettingsShowing)() override;
  private:
    std::weak_ptr<com_client::NotificationWindowImpl_Data> m_data;
  };

  class NotificationWindowImpl : public ukot::ie_web_notifications::INotificationWindow {
  public:
    NotificationWindowImpl(const std::function<void(bool clicked)>& onWindowDestroyed,
      const std::function<void()>& onShowSettingsRequested);
    void destroy() override;
    void setNotificationBody(const NotificationBody& notificationBody) override;
    void setSlot(uint8_t slotNumber) override;
    static NotificationWindowPtr create(com::INotificationWindowFactory& notificationWindowFactory,
      const NotificationBody& notificationBody, const std::function<void(bool clicked)>& onWindowDestroyed,
      const std::function<void()>& onShowSettingsRequested);
  private:
    std::shared_ptr<NotificationWindowImpl_Data> m_data;
    ATL::CComPtr<com::INotificationWindow> m_comWindow;
  };
}}}