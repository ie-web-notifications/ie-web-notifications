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
#include "ie_web_notifications_i.h"
#include <ExDispid.h>
#include "IClient.h"
#include <sergz.atl/pch-common.h>
// wtl headers
#include <atlapp.h>
#include <atlframe.h>
#include <atlmisc.h>
#include <atlctrls.h>
#include <atlcrack.h>
#include <mutex>
#include <map>
#include "WebBrowserEventsListener.h"
#include <sergz.utils/DPIAwareness.h>

namespace ukot { namespace ie_web_notifications {

  class AllowDenyButton : public ATL::CWindowImpl<AllowDenyButton, WTL::CButton> {
  public:
    DECLARE_WND_SUPERCLASS(NULL, CButton::GetWndClassName())
    std::function<void()> clicked;
    BEGIN_MSG_MAP(AllowDenyButton)
      MESSAGE_HANDLER(WM_LBUTTONDOWN, OnClick)
    END_MSG_MAP()

    LRESULT OnClick(UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*handled*/);
  };

  class CloseRequestPermissionWindowButton : public ATL::CWindowImpl<CloseRequestPermissionWindowButton, WTL::CButton>{
  public:
    DECLARE_WND_SUPERCLASS(NULL, CButton::GetWndClassName())
    std::function<void()> clicked;
    BEGIN_MSG_MAP(CloseRequestPermissionWindowButton)
      MESSAGE_HANDLER(WM_LBUTTONDOWN, OnClick)
    END_MSG_MAP()
  private:
    LRESULT OnClick(UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*handled*/);
  };

  class RequestPermissionWindow : public ATL::CWindowImpl<RequestPermissionWindow>, protected utils::DpiAwareness {
    struct PrivateArgCtr{};
  public:
    RequestPermissionWindow(const std::wstring& origin, const PrivateArgCtr&);
    std::function<void(Permission value)> selected;
    std::function<void()> destoyed;
    BEGIN_MSG_MAP(RequestPermissionWindow)
      MSG_WM_CREATE(OnCreate)
      MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColor)
      MESSAGE_HANDLER(WM_PAINT, OnPaint)
    END_MSG_MAP()
    static std::unique_ptr<RequestPermissionWindow> CreateX(const std::wstring& origin, HWND hTabWnd);
    uint32_t getDesiredHeight() const;
  private:
    void OnFinalMessage(HWND) override;
    LRESULT OnCreate(const CREATESTRUCT* /*createStruct*/);
    LRESULT OnCtlColor(UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*handled*/);
    LRESULT OnPaint(UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*handled*/);

  private:
    WTL::CFont m_titleFont;
    WTL::CFont m_btnFont;
    WTL::CFont m_closeBtnFont;
    AllowDenyButton m_allowBtn;
    AllowDenyButton m_denyBtn;
    CloseRequestPermissionWindowButton m_closeBtn;
    WTL::CStatic m_title;
    std::wstring m_origin;
    WTL::CBrush m_bkgndBrush;
  };

  class ATL_NO_VTABLE BHOImpl :
    public ATL::CComObjectRootEx<ATL::CComSingleThreadModel>,
    public ATL::CComCoClass<BHOImpl, &__uuidof(BHO)>,
    public ATL::IObjectWithSiteImpl<BHOImpl>,
    public ATL::IDispEventImpl<1, BHOImpl, &DIID_DWebBrowserEvents2, &LIBID_SHDocVw, 1, 1>
  {
  public:
    BHOImpl();
    ~BHOImpl();

    DECLARE_REGISTRY_RESOURCEID(IDR_REGISTRY_SCRIPT)

    DECLARE_NOT_AGGREGATABLE(BHOImpl)

    BEGIN_COM_MAP(BHOImpl)
      COM_INTERFACE_ENTRY(IObjectWithSite)
    END_COM_MAP()

    BEGIN_SINK_MAP(BHOImpl)
      SINK_ENTRY_EX(1, DIID_DWebBrowserEvents2, DISPID_BEFORENAVIGATE2, OnBeforeNavigate2)
      SINK_ENTRY_EX(1, DIID_DWebBrowserEvents2, DISPID_ONQUIT, OnOnQuit)
    END_SINK_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    HRESULT FinalConstruct();
    void FinalRelease();

  public:
    // IObjectWithSite
    STDMETHOD(SetSite)(IUnknown* pUnkSite) override;

    // DWebBrowserEvents2
    void STDMETHODCALLTYPE OnBeforeNavigate2(/* [in] */ IDispatch* pDisp,
      /* [in] */ VARIANT* URL,
      /* [in] */ VARIANT* Flags,
      /* [in] */ VARIANT* TargetFrameName,
      /* [in] */ VARIANT* PostData,
      /* [in] */ VARIANT* Headers,
      /* [in, out] */ VARIANT_BOOL* Cancel);
    void STDMETHODCALLTYPE OnOnQuit();
  private:
    void initInstance();
    void uninitInstance();
    static LRESULT CALLBACK SubclassFrameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
      UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
  private:
    std::shared_ptr<sergz::ILoggerFactory> m_loggerFactory;
    std::unique_ptr<sergz::ILogger> m_logger;
    ATL::CComPtr<IConnectionPoint> m_connectionPoint;
    ClientPtr m_client;
    struct Data
    {
       Data() : hTabWnd(nullptr), childMsgWindow(nullptr){}
      ~Data() {
        if (requestPermissionWindow) {
          requestPermissionWindow->DestroyWindow();
        }
      }
      std::shared_ptr<sergz::ILoggerFactory> loggerFactory;
      std::map<IWebBrowser2*, WebBrowserEventsListener*> connectedWebBrowsersCache;
      ATL::CComPtr<IWebBrowser2> webBrowser2;
      std::recursive_mutex m_pendingCallsMutex;
      std::vector<std::function<void()>> m_pendingCalls;
      HWND hTabWnd;
      HWND childMsgWindow;
      std::function<void(const std::function<void()>& call, bool)> dispatchCall;
      std::function<void(const std::function<void(const std::wstring& origin, Permission)>&)> requestPermission;
      std::function<std::wstring()> getOrigin;
      ClientPtr client;
      std::unique_ptr<RequestPermissionWindow> requestPermissionWindow;
    };
    // we need to have it as a shared pointer to get weak pointer to it to avoid
    // wrong usage after destroying of this class.
    std::shared_ptr<Data> m_data;
  };
}}