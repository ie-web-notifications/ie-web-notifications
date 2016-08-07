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
#include "D2DNotificationWindowFactoryImpl.h"
#include "D2DNotificationWindowImpl.h"

using namespace ukot::ie_web_notifications;

OBJECT_ENTRY_AUTO(__uuidof(com::D2DNotificationWindowFactory), D2DNotificationWindowFactoryImpl)

// uses defined `hr` of HRESULT type.
#define RETURN_ON_FAIL(expr) do {hr = (expr); if (FAILED(hr)) return hr;} while(false);
#define RETURN_VOID_ON_FAIL(expr) do {hr = (expr); assert(SUCCEEDED(hr) && #expr); if (FAILED(hr)) return;} while(false);

D2DNotificationWindowFactoryImpl::D2DNotificationWindowFactoryImpl()
{
}

D2DNotificationWindowFactoryImpl::~D2DNotificationWindowFactoryImpl()
{
}

HRESULT D2DNotificationWindowFactoryImpl::FinalConstruct() {
  HRESULT hr = S_OK;
  // Hack: use CLSID_WICImagingFactory1 to support Win7
  // Google "Why using WIC in my 32 bit application fails in Windows 7 32 bit"
  // TODO: It seems it would be better to manually detect the version of Windows and choose the
  // correct ID.
  RETURN_ON_FAIL(m_d2d.wiciFactory.CoCreateInstance(CLSID_WICImagingFactory1, nullptr, CLSCTX_INPROC_SERVER));
  RETURN_ON_FAIL(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_d2d.d2dFactory));
  ATL::CComPtr<IUnknown> dwriteFactoryUnk;
  RETURN_ON_FAIL(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), &dwriteFactoryUnk));
  RETURN_ON_FAIL(dwriteFactoryUnk.QueryInterface(&m_d2d.dwriteFactory));
  wchar_t locale[LOCALE_NAME_MAX_LENGTH];
  auto localeLength = GetUserDefaultLocaleName(locale, _countof(locale));
  if (!(localeLength > 0)) {
    return E_FAIL;
  }
  RETURN_ON_FAIL(m_d2d.dwriteFactory->CreateTextFormat(L"Segoe UI", /*system font collection*/nullptr,
    DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL,
    DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL, /*font size, dip*/14.f, locale, &m_d2d.titleTextFormat));
  RETURN_ON_FAIL(m_d2d.titleTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING::DWRITE_WORD_WRAPPING_NO_WRAP));
  RETURN_ON_FAIL(m_d2d.dwriteFactory->CreateTextFormat(L"Segoe UI", /*system font collection*/nullptr,
    DWRITE_FONT_WEIGHT::DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE::DWRITE_FONT_STYLE_NORMAL,
    DWRITE_FONT_STRETCH::DWRITE_FONT_STRETCH_NORMAL, /*font size, dip*/12.f, locale, &m_d2d.bodyTextFormat));
  {
    // The first call is only to support OS before Win8.1. It should work everywhere.
    RETURN_ON_FAIL(m_d2d.bodyTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING::DWRITE_WORD_WRAPPING_WRAP));
    // Ignore return value because it does not work on Win7, for instance.
    m_d2d.bodyTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING::DWRITE_WORD_WRAPPING_EMERGENCY_BREAK);
  }

  return hr;
}

void D2DNotificationWindowFactoryImpl::FinalRelease() {
}

STDMETHODIMP D2DNotificationWindowFactoryImpl::create(BSTR title, BSTR body, BSTR iconPath,
  com::INotificationWindowCallback* callback, com::INotificationWindow **retValue)
{
  if (!title || !body || !iconPath || !callback) {
    return E_POINTER;
  }

  if (!retValue || *retValue != nullptr) {
    return E_POINTER;
  }
  ATL::CComPtr<com::INotificationWindow> notificationWindow;
  NotificationBody notificationBody = NotificationBody::create(title, body, iconPath);
  HRESULT hr = ukot::atl::SharedObject<D2DNotificationWindowImpl>::Create(&notificationWindow, m_d2d, notificationBody, callback);
  *retValue = notificationWindow.Detach();
  return hr;
}

STDMETHODIMP D2DNotificationWindowFactoryImpl::InterfaceSupportsErrorInfo(REFIID riid) {
  static const IID* const arr[] = 
  {
    &__uuidof(com::INotificationWindowFactory)
  };

  for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
  {
    if (InlineIsEqualGUID(*arr[i],riid))
      return S_OK;
  }
  return S_FALSE;
}

