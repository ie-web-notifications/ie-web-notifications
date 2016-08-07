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
#include "../ie_web_notificationstoast_i.h"
#include <roapi.h>
#include <wrl/client.h>
#include <wrl/implements.h>
#include <windows.ui.notifications.h>
#include "../StringReferenceWrapper.h"
#include <ie_web_notifications.utils/NotificationBody.h>

namespace ukot { namespace ie_web_notifications {
  class
    ATL_NO_VTABLE
    DECLSPEC_UUID("C4F748C9-96A5-4546-B79C-241A5E351BE2")
  ToastNotificationWindowImpl :
    public ATL::CComObjectRootEx<ATL::CComMultiThreadModel>,
    public ATL::CComCoClass<ToastNotificationWindowImpl, &__uuidof(ToastNotificationWindowImpl)>,
    public ISupportErrorInfo,
    public com::INotificationWindow
  {
  public:
    ToastNotificationWindowImpl(const NotificationBody& notificationBody,
      com::INotificationWindowCallback* callback);
    ~ToastNotificationWindowImpl();

  DECLARE_NO_REGISTRY()
  DECLARE_NOT_AGGREGATABLE(ToastNotificationWindowImpl)

  BEGIN_COM_MAP(ToastNotificationWindowImpl)
    COM_INTERFACE_ENTRY(com::INotificationWindow)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
  END_COM_MAP()

  // ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    HRESULT FinalConstruct();
    void FinalRelease();
  public:
    STDMETHOD(setNotificationBody)(BSTR title, BSTR body, BSTR iconPath) override {
      return E_NOTIMPL;
    }
    STDMETHOD(destroy)() override {
      return E_NOTIMPL;
    }
    STDMETHOD(setSlot)(byte slotNumber) override {
      return E_NOTIMPL;
    }
  private:
    HRESULT createToastXml(ABI::Windows::UI::Notifications::IToastNotificationManagerStatics& toastManager,
      _Outptr_ ABI::Windows::Data::Xml::Dom::IXmlDocument** inputXml);
    HRESULT setImageSrc(ABI::Windows::Data::Xml::Dom::IXmlDocument& toastXml);
  private:
    NotificationBody m_notificationBody;
    ATL::CComPtr<com::INotificationWindowCallback> m_notificationWindowCallback;
  };
}}