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
#include "../resource.h"
#include "../ie_web_notificationsd2d_i.h"
#include "D2DNotificationWindow.h"

namespace ukot { namespace ie_web_notifications {
  class
    ATL_NO_VTABLE
    DECLSPEC_UUID("B711FAD4-1850-4221-BB31-8791CA9FAC00")
  D2DNotificationWindowImpl :
    public ATL::CComObjectRootEx<ATL::CComSingleThreadModel>,
    public ATL::CComCoClass<D2DNotificationWindowImpl, &__uuidof(D2DNotificationWindowImpl)>,
    public ISupportErrorInfo,
    public com::INotificationWindow
  {
  public:
    D2DNotificationWindowImpl(const D2DDeviceIndependentResources& d2dDIPResources,
      const NotificationBody& notificationBody,
      com::INotificationWindowCallback* callback);
    ~D2DNotificationWindowImpl();

    DECLARE_PROTECT_FINAL_CONSTRUCT()
    DECLARE_NO_REGISTRY()
    DECLARE_NOT_AGGREGATABLE(D2DNotificationWindowImpl)

    BEGIN_COM_MAP(D2DNotificationWindowImpl)
      COM_INTERFACE_ENTRY(com::INotificationWindow)
      COM_INTERFACE_ENTRY(ISupportErrorInfo)
    END_COM_MAP()

    // ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    HRESULT FinalConstruct();
    void FinalRelease();

  public:
    STDMETHOD(setNotificationBody)(BSTR title, BSTR body, BSTR iconPath) override;
    STDMETHOD(destroy)() override;
    STDMETHOD(setSlot)(byte slotNumber) override;
  private:
    D2DNotificationBorderWindow m_notificationWindow;
    ATL::CComPtr<com::INotificationWindowCallback> m_notificationWindowCallback;
  };
}}