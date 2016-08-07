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
#include "ToastNotificationWindowFactoryImpl.h"
#include "ToastNotificationWindow.h"
#include <roapi.h>

using namespace ukot::ie_web_notifications;

OBJECT_ENTRY_AUTO(__uuidof(com::ToastNotificationWindowFactory), ToastNotificationWindowFactoryImpl)

namespace ukot { namespace ie_web_notifications {
  HRESULT tryCreateShortcut();
}}

ToastNotificationWindowFactoryImpl::ToastNotificationWindowFactoryImpl()
{

}

ToastNotificationWindowFactoryImpl::~ToastNotificationWindowFactoryImpl() {
}

STDMETHODIMP ToastNotificationWindowFactoryImpl::InterfaceSupportsErrorInfo(REFIID riid) {
  static const IID* const arr[] =
  {
    &__uuidof(com::INotificationWindowFactory)
  };

  for (int i = 0; i < sizeof(arr) / sizeof(arr[0]); i++)
  {
    if (InlineIsEqualGUID(*arr[i], riid))
      return S_OK;
  }
  return S_FALSE;
}

HRESULT ToastNotificationWindowFactoryImpl::FinalConstruct() {
  HRESULT hr = Windows::Foundation::Initialize(RO_INIT_MULTITHREADED);
  m_roInitialized = SUCCEEDED(hr);
  ::ukot::ie_web_notifications::tryCreateShortcut();
  return hr;
}

void ToastNotificationWindowFactoryImpl::FinalRelease() {
  if (m_roInitialized) {
    Windows::Foundation::Uninitialize();
  }
}

STDMETHODIMP ToastNotificationWindowFactoryImpl::create(BSTR title, BSTR body, BSTR iconPath,
  com::INotificationWindowCallback* callback, com::INotificationWindow** retValue)
{
  if (!title || !body || !iconPath || !callback) {
    return E_POINTER;
  }

  if (!retValue || *retValue != nullptr) {
    return E_POINTER;
  }
  NotificationBody notificationBody = NotificationBody::create(title, body, iconPath);
  HRESULT hr = ukot::atl::SharedObject<ToastNotificationWindowImpl>::Create(retValue, notificationBody, callback);
  return hr;
}