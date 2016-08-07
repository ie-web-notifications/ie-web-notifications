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

#pragma once
#include <functional>
#include <memory>
#include <ExDisp.h>
#include <ExDispid.h>
#include <MsHTML.h>
#include <MsHtmdid.h>

namespace ukot { namespace ie_web_notifications {
  class WebBrowserEventsListener;
  typedef ATL::IDispEventImpl <1, WebBrowserEventsListener,
    &__uuidof(DWebBrowserEvents2), &LIBID_SHDocVw, 1, 1> WebBrowserEvents2Listener;

  typedef ATL::IDispEventImpl <2, WebBrowserEventsListener,
    &__uuidof(HTMLDocumentEvents2), &LIBID_MSHTML, 4, 0> HTMLDocumentEvents2Listener;

  enum class WebBrowserEventsListenerState {
    FirstTimeLoading, Loading, Loaded
  };

  class ATL_NO_VTABLE WebBrowserEventsListener :
    public ATL::CComObjectRootEx<ATL::CComSingleThreadModel>,
    public WebBrowserEvents2Listener,
    public HTMLDocumentEvents2Listener,
    public IUnknown
  {
    typedef WebBrowserEventsListenerState State;
  public:
    typedef std::function<void()> OnDestroy;
    typedef std::function<void()> OnReloaded;
  
    WebBrowserEventsListener();
    ~WebBrowserEventsListener();
    BEGIN_COM_MAP(WebBrowserEventsListener)
      COM_INTERFACE_ENTRY(IUnknown)
    END_COM_MAP()
  
    DECLARE_NOT_AGGREGATABLE(WebBrowserEventsListener)
    BEGIN_SINK_MAP(WebBrowserEventsListener)
      SINK_ENTRY_EX(1, __uuidof(DWebBrowserEvents2), DISPID_DOCUMENTCOMPLETE, OnDocumentComplete)
      SINK_ENTRY_EX(1, __uuidof(DWebBrowserEvents2), DISPID_ONQUIT, OnOnQuit)
      SINK_ENTRY_EX(2, __uuidof(HTMLDocumentEvents2), DISPID_HTMLDOCUMENTEVENTS2_ONREADYSTATECHANGE, OnReadyStateChange)
    END_SINK_MAP()
  
    HRESULT STDMETHODCALLTYPE OnDocumentComplete(IDispatch* pDisp, VARIANT* urlVariant);
    void STDMETHODCALLTYPE OnOnQuit();
    void STDMETHODCALLTYPE OnReadyStateChange(IHTMLEventObj* pEvtObj);
  
    DECLARE_PROTECT_FINAL_CONSTRUCT()
  
    HRESULT FinalConstruct(){ return S_OK; }
    void FinalRelease();
    HRESULT Init(IWebBrowser2* webBrowser, const OnDestroy& onDestroy, const OnReloaded& onReloaded);

  private:
    void emitReloaded();
  private:
    ATL::CComPtr<IWebBrowser2> m_browser;
    OnDestroy m_onDestroy;
    OnReloaded m_onReloaded;
    bool m_isDocumentEvents2Connected;
    State m_state;
    std::shared_ptr<sergz::ILogger> m_logger;
  };
}}