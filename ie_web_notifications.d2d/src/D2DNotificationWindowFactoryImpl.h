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
#include "../resource.h"       // main symbols
#include "../ie_web_notificationsd2d_i.h"
#include "D2DDeviceIndependentResources.h"

namespace ukot { namespace ie_web_notifications {
  class ATL_NO_VTABLE D2DNotificationWindowFactoryImpl :
    public ATL::CComObjectRootEx<ATL::CComSingleThreadModel>,
    public ATL::CComCoClass<D2DNotificationWindowFactoryImpl, &__uuidof(com::D2DNotificationWindowFactory)>,
    public ISupportErrorInfo,
    public com::INotificationWindowFactory
  {
  public:
    D2DNotificationWindowFactoryImpl();
    ~D2DNotificationWindowFactoryImpl();

    DECLARE_PROTECT_FINAL_CONSTRUCT()
    DECLARE_NO_REGISTRY()
    DECLARE_NOT_AGGREGATABLE(D2DNotificationWindowFactoryImpl)

    BEGIN_COM_MAP(D2DNotificationWindowFactoryImpl)
      COM_INTERFACE_ENTRY(com::INotificationWindowFactory)
      COM_INTERFACE_ENTRY(ISupportErrorInfo)
    END_COM_MAP()

  // ISupportsErrorInfo
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    HRESULT FinalConstruct();
    void FinalRelease();

  public:
    STDMETHOD(create)(BSTR title, BSTR body, BSTR iconPath, com::INotificationWindowCallback* callback,
      com::INotificationWindow **retValue) override;
  private:
    D2DDeviceIndependentResources m_d2d;
  };
}}