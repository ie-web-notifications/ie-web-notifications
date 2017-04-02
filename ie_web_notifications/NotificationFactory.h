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
#include "IDispatchEmptyImpl.h"
#include <sergz.utils/CScopedLocale.h>
#include "IClient.h"

namespace ukot { namespace ie_web_notifications {
  struct DECLSPEC_UUID("B711FAD4-1850-4221-BB31-8791CA9FAC01")
  DECLSPEC_NOVTABLE INotificationFactory : public IUnknown {
    STDMETHOD(beforeUnload)() = 0;
  };
  class ATL_NO_VTABLE NotificationFactoryImpl
    : public ATL::CComObjectRootEx<ATL::CComSingleThreadModel>
    , public IDispatchExEmptyImpl
    , public INotificationFactory
  {
    struct Context {
      std::function<void(const std::function<void()>&, bool)> dispatchCall;
      ClientPtr client;
      ATL::CComPtr<IDispatchEx> fireEventHelper;
      // notification javascript objects. They are created by IE.
      std::vector<ATL::CComPtr<IDispatchEx>> jsObjects;
    };
  public:
    typedef std::function<void(const std::function<void(const std::wstring& origin, Permission)>&)> RequestPermission;
    NotificationFactoryImpl(const ClientPtr& client
      , const std::function<void(const std::function<void()>&, bool)>& dispatchCall
      , const std::function<std::wstring()>& getOrigin
      , const RequestPermission& requestPermission);
    ~NotificationFactoryImpl();

    DECLARE_NOT_AGGREGATABLE(NotificationFactoryImpl)

    BEGIN_COM_MAP(NotificationFactoryImpl)
      COM_INTERFACE_ENTRY(IDispatch)
      COM_INTERFACE_ENTRY(IDispatchEx)
      COM_INTERFACE_ENTRY(INotificationFactory)
    END_COM_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    HRESULT FinalConstruct();
    void FinalRelease();

    STDMETHOD(GetDispID)(/* [in] */ __RPC__in BSTR bstrName, /* [in] */ DWORD grfdex,
      /* [out] */ __RPC__out DISPID *pid) override;

    STDMETHOD(InvokeEx)(/* [annotation][in] */ _In_  DISPID id,
      /* [annotation][in] */ _In_  LCID lcid,
      /* [annotation][in] */ _In_  WORD wFlags,
      /* [annotation][in] */ _In_  DISPPARAMS *pdp,
      /* [annotation][out] */ _Out_opt_  VARIANT *pvarRes,
      /* [annotation][out] */ _Out_opt_  EXCEPINFO *pei,
      /* [annotation][unique][in] */ _In_opt_  IServiceProvider *pspCaller) override;
    STDMETHOD(beforeUnload)() override;

  private:
    static void fireEvent(Context& context, IDispatchEx& jsObject, const wchar_t* nativeEvent);
    NotificationId sendNotification(IDispatchEx& _this, const std::wstring& title,
      const ATL::CComPtr<IDispatchEx>& notificationOptions, IServiceProvider* serviceProvider);
  private:
    std::unique_ptr<sergz::ILogger> m_logger;
    ATL::CComPtr<IDispatchEx> m_notificationPrototype;
    ukot::util::CScopedLocale m_LC_ALL_C;
    std::function<std::wstring()> m_getOrigin;
    RequestPermission m_requestPermission;
    std::shared_ptr<Context> m_context;
  };
}}