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
#include "IWebBrowser2Helper.h"
#include <array>
#include <ie_web_notifications.utils/convertTo.h>

std::wstring ukot::ie_web_notifications::getOrigin(IWebBrowser2& browser) {
  std::wstring retValue;
  ATL::CComPtr<IDispatch> dispDocument;
  ATL::CComQIPtr<IHTMLDocument2> htmlDocument2;
  ATL::CComPtr<IHTMLLocation> htmlLocation;
  // TODO: try to get 'origin' property from IDispatch firstly
  if (FAILED(browser.get_Document(&dispDocument)) || !(htmlDocument2 = dispDocument)
    || FAILED(htmlDocument2->get_location(&htmlLocation)) || !htmlLocation) {
    return retValue;
  }
  ATL::CComBSTR host;
  ATL::CComBSTR protocol;
  if (FAILED(htmlLocation->get_host(&host)) || !host
    || FAILED(htmlLocation->get_protocol(&protocol) || !protocol)) {
    return retValue;
  }
  return util::convertToWString(protocol) + L"//" + util::convertToWString(host);
}

HWND ukot::ie_web_notifications::getTabHwnd(IWebBrowser2& browser) {
  SHANDLE_PTR browserWindow = 0;
  if (FAILED(browser.get_HWND(&browserWindow))) {
    return nullptr;
  }
  std::array<wchar_t, MAX_PATH> className;
  HWND hTabWnd = ::GetWindow(reinterpret_cast<HWND>(browserWindow), GW_CHILD);
  while (hTabWnd)
  {
    className[0] = L'\0';
    int classNameLength = GetClassNameW(hTabWnd, className.data(), className.size());
    bool isFrameTabClass = false;
    if (classNameLength
      && (wcscmp(className.data(), L"TabWindowClass") == 0
      || (isFrameTabClass = wcscmp(className.data(), L"Frame Tab") == 0)))
    {
      // IE8 support
      HWND hTabWnd2 = hTabWnd;
      if (isFrameTabClass
        && (hTabWnd2 = ::FindWindowEx(hTabWnd2, NULL, L"TabWindowClass", NULL)))
      {
        DWORD nProcessId = 0;
        ::GetWindowThreadProcessId(hTabWnd2, &nProcessId);
        if (::GetCurrentProcessId() == nProcessId)
        {
          hTabWnd = hTabWnd2;
          break;
        }
      }
    }
    hTabWnd = ::GetWindow(hTabWnd, GW_HWNDNEXT);
  }
  return hTabWnd;
}

std::wstring ukot::ie_web_notifications::getLocationUrl(IWebBrowser2& browser) {
  ATL::CComBSTR locationUrl;
  if (FAILED(browser.get_LocationURL(&locationUrl)) || !locationUrl) {
    return std::wstring();
  }
  return util::convertToWString(locationUrl);
}

ATL::CComPtr<IHTMLLocation> ukot::ie_web_notifications::getHtmlLocation(const ATL::CComPtr<IWebBrowser2>& webBrowser2) {
  if (!webBrowser2)
    return nullptr;
  ATL::CComPtr<IDispatch> pDocDispatch;
  webBrowser2->get_Document(&pDocDispatch);
  ATL::CComQIPtr<IHTMLDocument2> pDoc2 = pDocDispatch;
  if (!pDoc2) {
    return nullptr;
  }
  ATL::CComPtr<IHTMLWindow2> htmlWindow2;
  if (FAILED(pDoc2->get_parentWindow(&htmlWindow2)) || !htmlWindow2) {
    return nullptr;
  }
  ATL::CComPtr<IHTMLLocation> htmlLocation;
  if (FAILED(htmlWindow2->get_location(&htmlLocation))) {
    return nullptr;
  }
  return htmlLocation;
}