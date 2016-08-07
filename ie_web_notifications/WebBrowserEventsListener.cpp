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

// This file is based on Adblock Plus for IE project(licensed under GPLv3),
// who gave permission to redistribute it under the BSD 3 - clause license."

#include "stdafx.h"
#include "WebBrowserEventsListener.h"
#include "IWebBrowser2Helper.h"
#include "LoggerHelper.h"
#include <ie_web_notifications.utils/convertTo.h>

using namespace ukot::ie_web_notifications;

namespace {
  std::wstring StateToString(WebBrowserEventsListenerState state) {
    switch (state) {
    case ukot::ie_web_notifications::WebBrowserEventsListenerState::FirstTimeLoading:
      return L"FirstTimeLoading";
    case ukot::ie_web_notifications::WebBrowserEventsListenerState::Loading:
      return L"Loading";
    case ukot::ie_web_notifications::WebBrowserEventsListenerState::Loaded:
      return L"Loaded";
    default:
      ;
    }
    return L"Unknown";
  }
}

WebBrowserEventsListener::WebBrowserEventsListener()
  : m_isDocumentEvents2Connected(false)
  , m_state(State::FirstTimeLoading)
{
  std::wstringstream ss;
  ss << this << L".Listener::";
  std::wstring thisStr = ss.str();
  m_logger = createOutputDebugStringLoggerImpl([thisStr]()->std::wstring{
    return thisStr;
  });
}

WebBrowserEventsListener::~WebBrowserEventsListener()
{
}

HRESULT STDMETHODCALLTYPE WebBrowserEventsListener::OnDocumentComplete(IDispatch* dispFrameBrowser, VARIANT* variantUrl)
{
  if (!dispFrameBrowser) {
    return E_POINTER;
  }

  // if it's a signal from another browser (sub-frame for-example) then ignore it.
  if (!m_browser.IsEqualObject(dispFrameBrowser)) {
    return S_OK;
  }

  if (variantUrl && VT_BSTR == variantUrl->vt) {
    SERGZ_DEBUG() << getLocationUrl(*m_browser) << L" >> " << util::convertToWString(variantUrl->bstrVal);
  }

  if (!m_isDocumentEvents2Connected) {
    ATL::CComPtr<IDispatch> dispDocument;
    ATL::CComQIPtr<IHTMLDocument2> htmlDocument2;
    bool isHtmlDocument2 = SUCCEEDED(m_browser->get_Document(&dispDocument)) && (htmlDocument2 = dispDocument);
    isHtmlDocument2 && (m_isDocumentEvents2Connected = SUCCEEDED(HTMLDocumentEvents2Listener::DispEventAdvise(htmlDocument2)));
  }

  SERGZ_DEBUG() << StateToString(m_state);
  if (m_state == State::FirstTimeLoading) {
    m_state = State::Loaded;
    emitReloaded();
  }
  return S_OK;
}

void STDMETHODCALLTYPE WebBrowserEventsListener::OnOnQuit() {
  if (m_isDocumentEvents2Connected) {
    ATL::CComPtr<IDispatch> dispDocument;
    ATL::CComQIPtr<IHTMLDocument2> htmlDocument2;
    if (SUCCEEDED(m_browser->get_Document(&dispDocument)) && (htmlDocument2 = dispDocument)) {
      HTMLDocumentEvents2Listener::DispEventUnadvise(htmlDocument2);
    }
  }
  WebBrowserEvents2Listener::DispEventUnadvise(m_browser);
}

void STDMETHODCALLTYPE WebBrowserEventsListener::OnReadyStateChange(IHTMLEventObj* /*pEvtObj*/) {
  auto documentReadyState = [this]()->std::wstring {
    std::wstring notAvailableReadyState;
    ATL::CComPtr<IDispatch> pDocDispatch;
    m_browser->get_Document(&pDocDispatch);
    ATL::CComQIPtr<IHTMLDocument2> htmlDocument2 = pDocDispatch;
    if (!htmlDocument2) {
      assert(false && "htmlDocument2 in OnReadyStateChange should not be nullptr");
      return notAvailableReadyState;
    }
    ATL::CComBSTR readyState;
    if (FAILED(htmlDocument2->get_readyState(&readyState)) || !readyState) {
      assert(false && "cannot obtain document readyState in OnReadyStateChange");
      return notAvailableReadyState;
    }
    return util::convertToWString(readyState);
  }();
  SERGZ_DEBUG() << L"Document ready state => " << documentReadyState << getLocationUrl(*m_browser);
  if (documentReadyState == L"loading") {
    m_state = State::Loading;
  } else if (documentReadyState == L"interactive") {
  }
  else if (documentReadyState == L"complete") {
    if (m_state == State::Loading) {
      m_state = State::Loaded;
      emitReloaded();
    } else if (m_state == State::Loaded) {
    } else {
      assert(false);
    }
  } else if (documentReadyState == L"uninitialized") {
  } else {
    assert(false);
  }
}

void WebBrowserEventsListener::FinalRelease() {
  SERGZ_DEBUG() << L"Destorying";
  if (!!m_onDestroy) {
    m_onDestroy();
  }
}

HRESULT WebBrowserEventsListener::Init(IWebBrowser2* webBrowser, const OnDestroy& onDestroy, const OnReloaded& onReloaded) {
  if (!(m_browser = webBrowser)) {
    return E_POINTER;
  }
  SERGZ_DEBUG() << m_browser << L" Initialized::" << getLocationUrl(*m_browser);
  m_onDestroy = onDestroy;
  m_onReloaded = onReloaded;
  if (FAILED(WebBrowserEvents2Listener::DispEventAdvise(m_browser, &DIID_DWebBrowserEvents2))) {
    return E_FAIL;
  }
  return S_OK;
}

void WebBrowserEventsListener::emitReloaded() {
  SERGZ_DEBUG() << StateToString(m_state) << L"::" << getLocationUrl(*m_browser);
  if (!!m_onReloaded) {
    m_onReloaded();
  }
}
