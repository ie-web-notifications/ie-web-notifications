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
#include "D2DNotificationWindowImpl.h"

#define UKOT_ATL_ENSURE(condition, message, code) do { if (!(condition)) return Error(message, /*iid*/GUID_NULL, code); } while(false);

using namespace ukot::ie_web_notifications;

D2DNotificationWindowImpl::D2DNotificationWindowImpl(const D2DDeviceIndependentResources& d2dDIPResources,
  const NotificationBody& notificationBody, com::INotificationWindowCallback* callback)
  : m_notificationWindow(d2dDIPResources, notificationBody)
  , m_notificationWindowCallback(callback)
{
}

D2DNotificationWindowImpl::~D2DNotificationWindowImpl() {

}

HRESULT D2DNotificationWindowImpl::FinalConstruct() {
  UKOT_ATL_ENSURE(!!m_notificationWindowCallback, L"INotificationWindowCallback must be not nullptr", E_POINTER);
  m_notificationWindow.setOnDestroyed([this](bool clicked){
    m_notificationWindowCallback->closed(clicked ? VARIANT_TRUE : VARIANT_FALSE);
  });
  m_notificationWindow.setOnSettingsRequested([this]{
    m_notificationWindowCallback->requestSettingsShowing();
  });
  m_notificationWindow.Create(nullptr);
  UKOT_ATL_ENSURE(m_notificationWindow.operator HWND() != nullptr, L"Notification Window HWND should not be nullptr", E_HANDLE);
  m_notificationWindow.ShowWindow(SW_SHOWNOACTIVATE);
  m_notificationWindow.UpdateWindow();
  return S_OK;
}

void D2DNotificationWindowImpl::FinalRelease() {

}

STDMETHODIMP D2DNotificationWindowImpl::setNotificationBody(BSTR title, BSTR body, BSTR iconPath) {
  m_notificationWindow.setNotificationBody(NotificationBody::create(title, body, iconPath));
  return S_OK;
}

STDMETHODIMP D2DNotificationWindowImpl::destroy() {
  m_notificationWindow.DestroyWindow();
  return S_OK;
}

STDMETHODIMP D2DNotificationWindowImpl::setSlot(byte slotNumber) {
  m_notificationWindow.setSlot(slotNumber);
  return S_OK;
}

STDMETHODIMP D2DNotificationWindowImpl::InterfaceSupportsErrorInfo(REFIID riid) {
  static const IID* const arr[] =
  {
    &__uuidof(com::INotificationWindow)
  };

  for (int i = 0; i < sizeof(arr) / sizeof(arr[0]); i++)
  {
    if (InlineIsEqualGUID(*arr[i], riid))
      return S_OK;
  }
  return S_FALSE;
}