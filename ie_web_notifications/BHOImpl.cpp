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
#include "BHOImpl.h"
#include <sergz.logger/Logger.h>
#include <filesystem>
#include <mutex>
#include <iomanip>
#include <array>
#include <set>
#include <sergz.atl/atl.h>
#include <sergz.utils/utils.h>
#include <atlstr.h>
#include "NotificationFactory.h"
#include "GModel.h"
#include "IWebBrowser2Helper.h"
#include "LoggerHelper.h"

using namespace ukot::ie_web_notifications;

OBJECT_ENTRY_AUTO(__uuidof(ukot::ie_web_notifications::BHO), BHOImpl)

namespace {
  const auto kWM_DEQUEUE_CALL = WM_USER + 1;
}

LRESULT AllowDenyButton::OnClick(UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& handled) {
  if (!!clicked) {
    clicked();
  }
  handled = FALSE;
  return 0;
}

LRESULT CloseRequestPermissionWindowButton::OnClick(UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& handled) {
  if (!!clicked) {
    clicked();
  }
  handled = FALSE;
  return 0;
}

RequestPermissionWindow::RequestPermissionWindow(const std::wstring& origin, const PrivateArgCtr&)
  : m_origin(origin)
{
}

LRESULT RequestPermissionWindow::OnCreate(const CREATESTRUCT* /*createStruct*/) {
  {
    WTL::CDC hdc = GetDC();
    m_dpi = hdc.GetDeviceCaps(LOGPIXELSX);
  }
  m_bkgndBrush.CreateSolidBrush(RGB(255, 255, 255));
  uint32_t width = 50;
  uint32_t height = 20;
  DWORD childStyle = WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS;
  WTL::CRect titleRect{};
  auto titleText = L"Allow " + m_origin + L" to show desktop notifications?";
  m_title.Create(m_hWnd, titleRect, titleText.c_str(), childStyle, 0, 101);

  {
    LOGFONT logFont = { 0 };
    logFont.lfQuality = CLEARTYPE_QUALITY;
    logFont.lfHeight = 100;
    SecureHelper::strncpy_x(logFont.lfFaceName, _countof(logFont.lfFaceName), L"Segoe UI", _TRUNCATE);
    m_titleFont.CreatePointFontIndirect(&logFont);
    m_title.SetFont(m_titleFont);
  }

  {
    WTL::CDC hdc = m_title.GetDC();
    SIZE windowTextSize{};
    if (hdc.GetTextExtent(titleText.c_str(), static_cast<int>(titleText.length()), &windowTextSize))
    {
      titleRect = WTL::CRect{POINT{10, 6}, SIZE{windowTextSize.cx, windowTextSize.cy}};
      WTL::CRect moveWindowTo = DPIAware(titleRect);
      m_title.MoveWindow(&moveWindowTo, FALSE /* no repaint now */);
    }
  }

  {
    LOGFONT logFont = { 0 };
    logFont.lfQuality = CLEARTYPE_QUALITY;
    logFont.lfHeight = 84;
    SecureHelper::strncpy_x(logFont.lfFaceName, _countof(logFont.lfFaceName), L"Segoe UI", _TRUNCATE);
    m_btnFont.CreatePointFontIndirect(&logFont);
  }

  WTL::CRect allowBtnRect{POINT{titleRect.right, 6}, SIZE{width, height}};
  m_allowBtn.Create(m_hWnd, DPIAware(allowBtnRect), L"Allow");
  m_allowBtn.SetFont(m_btnFont);
  m_allowBtn.clicked = [this]{
    if (!!selected) {
      selected(Permission::granted);
    }
    this->PostMessage(WM_CLOSE);
  };
  WTL::CRect denyBtnRect{POINT{allowBtnRect.right + 10, 6}, SIZE{width, height}};
  m_denyBtn.Create(m_hWnd, DPIAware(denyBtnRect), L"Deny");
  m_denyBtn.SetFont(m_btnFont);

  m_denyBtn.clicked = [this]{
    if (!!selected) {
      selected(Permission::denied);
    }
    this->PostMessage(WM_CLOSE);
  };
  return 0;
}

uint32_t RequestPermissionWindow::getDesiredHeight() const {
  return DPIAware(32);
}

void RequestPermissionWindow::OnFinalMessage(HWND) {
  if (!!destoyed) {
    destoyed();
  }
}

LRESULT RequestPermissionWindow::OnCtlColor(UINT /*msg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& handled) {
  if (reinterpret_cast<HWND>(lParam) != m_title) {
    handled = FALSE;
  }
  return reinterpret_cast<LRESULT>(m_bkgndBrush.m_hBrush);
}

LRESULT RequestPermissionWindow::OnPaint(UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*handled*/) {
  WTL::CPaintDC dc(m_hWnd);
  WTL::CRect rect;
  GetClientRect(&rect);
  dc.DrawEdge(&rect, EDGE_BUMP, BF_BOTTOM);
  return 0;
}

std::unique_ptr<RequestPermissionWindow> RequestPermissionWindow::CreateX(const std::wstring& origin,
  HWND hTabWnd) {
  WTL::CRect tabRect{};
  ::GetClientRect(hTabWnd, &tabRect);
  std::unique_ptr<RequestPermissionWindow> requestPermissionWindow{new RequestPermissionWindow{origin, PrivateArgCtr{}}};
  requestPermissionWindow->Create(hTabWnd,
    WTL::CRect{WTL::CPoint{}, SIZE{tabRect.right - tabRect.left, 80}}, nullptr, WS_CHILD | WS_VISIBLE);
  tabRect.bottom = tabRect.top + requestPermissionWindow->getDesiredHeight();
  requestPermissionWindow->SetWindowPos(0, &tabRect, SWP_NOMOVE);
  return requestPermissionWindow;
}

BHOImpl::BHOImpl()
{
}

BHOImpl::~BHOImpl()
{
}

HRESULT BHOImpl::FinalConstruct() {
  try {
    m_client = GModel::instance().getClient();
    std::wstringstream ss;
    ss << this << L".BHO";
    auto loggerName = ss.str();
    m_logger = createOutputDebugStringLoggerImpl([loggerName]()->std::wstring{
      return loggerName;
    });
#if !SERGZ_LOGGER_DISABLED
    m_loggerFactory = createLoggerFactory();
    std::tr2::sys::wpath appPath = utils::getDllDirPath();
    std::tr2::sys::wpath loggerPropertiesPath = appPath; // there is no `operator /` so do it in two steps.
    loggerPropertiesPath /= k_loggerPropertiesFileName;
    std::tr2::sys::wpath localLowPath = utils::getAppDataLocalLowPath();
    if (exists(loggerPropertiesPath) && exists(localLowPath)) {
      auto appLocalLow_Niko = localLowPath;
      appLocalLow_Niko /= L"Niko";
      ILoggerFactory::Properties dataDirProperties{ { L"SERGZ_APP_DATA_DIR", appLocalLow_Niko } };
      m_loggerFactory->propertiesFile(loggerPropertiesPath, dataDirProperties);
    }
    m_logger = m_loggerFactory->getLogger(L"Niko");
#endif
  }
  catch (...){}
  return S_OK;
}

void BHOImpl::FinalRelease() {
  assert(!m_data && "internal shared data object should be nullptr");
  // JIC clean the everything in the proper order before destroying
  m_logger.reset();
  m_loggerFactory.reset();
}

void BHOImpl::initInstance() {
  SERGZ_TRACE();
  assert(!!m_data->webBrowser2 && "m_werBrowser should be initialized");
  if (!m_data->webBrowser2) {
    SERGZ_ERROR() << L"Cannot initialize the instance, the browser is not initialized";
    return;
  }

  m_data->hTabWnd = getTabHwnd(*m_data->webBrowser2);

  DWORD exWindowStyles = WS_EX_NOACTIVATE | WS_EX_NOINHERITLAYOUT | WS_EX_NOPARENTNOTIFY;// | WS_EX_NOREDIRECTIONBITMAP;
  auto className = L"ie_web_notifications.ChildMsgWindowClass";
  DWORD windowStyles = WS_CHILD | WS_DISABLED;
  m_data->childMsgWindow = ::CreateWindowEx(exWindowStyles, className, /*window name*/nullptr, windowStyles, /*x*/0, /*y*/0,
    /*width*/0, /*height*/0, m_data->hTabWnd, nullptr, ATL::_AtlBaseModule.GetModuleInstance(), nullptr);
  //TODO: if cannot create then undo the rest.
  if (nullptr == m_data->childMsgWindow) {
    DWORD errCode = GetLastError();
    std::string();
  }
  if (FALSE == SetWindowSubclass(m_data->childMsgWindow, &SubclassFrameWndProc,
    /*user Id subclass*/1, /*user data*/reinterpret_cast<DWORD_PTR>(this))) {
    auto lastError = GetLastError();
    SERGZ_ERROR() << L"Cannot subclass browser window " << static_cast<uint32_t>(lastError);
  }
  std::weak_ptr<Data> dataForCapturing = m_data;
  m_data->dispatchCall = [dataForCapturing](const std::function<void()>& call, bool clicked) {
    if (auto data = dataForCapturing.lock()) {
      {
        std::lock_guard<std::recursive_mutex> lock(data->m_pendingCallsMutex);
        data->m_pendingCalls.push_back(call);
      }
      // We cannot use SendMessage because MessageBox does not work. I guess it affects some message
      // loop related stuff.
      PostMessage(reinterpret_cast<HWND>(data->childMsgWindow), kWM_DEQUEUE_CALL, 0, 0);
      if (clicked) {
        // TODO: it does not work
        //ShowWindow(data->hTabWnd, SW_SHOWNORMAL);
      }
    }
  };
  m_data->getOrigin = [dataForCapturing]()->std::wstring {
    if (auto data = dataForCapturing.lock()) {
      return ::getOrigin(*data->webBrowser2);
    }
    return L"";
  };

  m_data->requestPermission = [dataForCapturing](const std::function<void(const std::wstring& origin, Permission)>& callback){
    if (auto data = dataForCapturing.lock()) {
      if (!!data->requestPermissionWindow) {
        return;
      }
      auto origin = getOrigin(*data->webBrowser2);
      data->requestPermissionWindow = RequestPermissionWindow::CreateX(origin, data->hTabWnd);
      data->requestPermissionWindow->destoyed = [dataForCapturing] {
        if (auto data = dataForCapturing.lock()) {
          data->requestPermissionWindow.reset();
        }
      };
      data->requestPermissionWindow->selected = [dataForCapturing, callback, origin](Permission value){
        if (auto data = dataForCapturing.lock()) {
          if (!callback) {
            return;
          }
          callback(origin, value);
        }
      };
    }
  };

  if (FAILED(DispEventAdvise(m_data->webBrowser2, &DIID_DWebBrowserEvents2)))
  {
    SERGZ_ERROR() << L"Cannot advise DIID_DWebBrowserEvents2";
    return;
  }
}

void BHOImpl::uninitInstance() {
  SERGZ_TRACE();
  if (nullptr != m_data->childMsgWindow) {
    DestroyWindow(m_data->childMsgWindow);
    m_data->childMsgWindow = nullptr;
  }
  DispEventUnadvise(m_data->webBrowser2, &DIID_DWebBrowserEvents2);
  assert(m_data->connectedWebBrowsersCache.empty() && "Connected web browser cache should be already empty");
}

STDMETHODIMP BHOImpl::SetSite(IUnknown* pUnkSite) {
  if (!m_client) {
    return S_OK;
  }
  try {
    if (nullptr != pUnkSite) {
      m_data = std::make_shared<Data>();
      m_data->loggerFactory = m_loggerFactory;
      m_data->client = m_client;
      if (FAILED(pUnkSite->QueryInterface(&m_data->webBrowser2))) {
        SERGZ_ERROR() << L"Cannot obtain the webbroser";
        return S_OK;
      }
      initInstance();
      SERGZ_INFO() << L"Loaded for the tab";
    } else {
      uninitInstance();
      m_data.reset();
      SERGZ_INFO() << L"UnLoaded for the tab";
    }
    return ATL::IObjectWithSiteImpl<BHOImpl>::SetSite(pUnkSite);
  } catch(...){}
  return S_OK;
}

LRESULT CALLBACK BHOImpl::SubclassFrameWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
  UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
  if (kWM_DEQUEUE_CALL == uMsg) {
    BHOImpl* _this = reinterpret_cast<BHOImpl*>(dwRefData);
    std::lock_guard<std::recursive_mutex> lock(_this->m_data->m_pendingCallsMutex);
    if (_this->m_data->m_pendingCalls.empty()) {
      return FALSE;
    }
    const auto& ii_call = _this->m_data->m_pendingCalls.begin();
    if (!!(*ii_call)) {
      (*ii_call)();
    }
    _this->m_data->m_pendingCalls.erase(ii_call);
    return TRUE;
  } else if (WM_SIZE == uMsg || WM_SIZING == uMsg) {
    BHOImpl* _this = reinterpret_cast<BHOImpl*>(dwRefData);
    if (_this->m_data->requestPermissionWindow) {
      uint32_t newWidth = 0;
      if (WM_SIZE == uMsg) {
        newWidth = LOWORD(lParam);
      } else if (WM_SIZING == uMsg) {
        RECT* rect = reinterpret_cast<RECT*>(lParam);
        newWidth = rect->right - rect->left;
      }
      WTL::CRect rect;
      _this->m_data->requestPermissionWindow->GetClientRect(&rect);
      rect.right = rect.left + newWidth;
      _this->m_data->requestPermissionWindow->MoveWindow(&rect);
    }
  }
  return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

namespace {
  class ATL_NO_VTABLE OnBeforeUnloadImpl :
    public ATL::CComObjectRootEx<ATL::CComSingleThreadModel>,
    public IDispatchExEmptyImpl
  {
  public:
    OnBeforeUnloadImpl(const ATL::CComPtr<INotificationFactory>& notifications)
      : m_notifications(notifications)
    {
    }
    DECLARE_NOT_AGGREGATABLE(OnBeforeUnloadImpl)

    BEGIN_COM_MAP(OnBeforeUnloadImpl)
      COM_INTERFACE_ENTRY(IDispatch)
      COM_INTERFACE_ENTRY(IDispatchEx)
    END_COM_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    STDMETHOD(InvokeEx)(
      /* [annotation][in] */
      _In_  DISPID id,
      /* [annotation][in] */
      _In_  LCID lcid,
      /* [annotation][in] */
      _In_  WORD wFlags,
      /* [annotation][in] */
      _In_  DISPPARAMS *pdp,
      /* [annotation][out] */
      _Out_opt_  VARIANT *pvarRes,
      /* [annotation][out] */
      _Out_opt_  EXCEPINFO *pei,
      /* [annotation][unique][in] */
      _In_opt_  IServiceProvider* pspCaller) override
    {
      if (0 == id && DISPATCH_METHOD == wFlags) {
        m_notifications->beforeUnload();
        return S_OK;
      }
      return E_NOTIMPL;
    }

    HRESULT FinalConstruct() {
      return S_OK;
    }
    void FinalRelease(){}
  private:
    ATL::CComPtr<INotificationFactory> m_notifications;
  };

  void InjectNotification(IWebBrowser2& webBrowser
    , const ClientPtr& client
    , const std::function<void(const std::function<void()>& call, bool)>& dispatchCall
    , const NotificationFactoryImpl::RequestPermission& requestPermission
    , const std::function<std::wstring()>& getOrigin
    , const std::shared_ptr<sergz::ILoggerFactory>& loggerFactory) {
    ATL::CComPtr<IDispatch> pDocDispatch;
    webBrowser.get_Document(&pDocDispatch);
    ATL::CComQIPtr<IHTMLDocument2> pDoc2 = pDocDispatch;
    if (!pDoc2) {
      return;
    }
    ATL::CComPtr<IHTMLWindow2> pWnd2;
    ATL::CComQIPtr<IDispatchEx> pWndEx = pWnd2;
    ATL::CComQIPtr<IEventTarget> windowEvents;
    pDoc2->get_parentWindow(&pWnd2);
    if (!(windowEvents = pWndEx = pWnd2)) {
      return;
    }

    DISPID dispid = 0;
    HRESULT hr = pWndEx->GetDispID(ATL::CComBSTR(L"Notification"), fdexNameCaseSensitive, &dispid);
    if (SUCCEEDED(hr) && 0 != dispid) {
      return;
    }
    hr = pWndEx->GetDispID(ATL::CComBSTR(L"Notification"), fdexNameEnsure, &dispid);
    if (FAILED(hr)) {
      return;
    }
    ATL::CComPtr<IDispatch> webNotification;
    hr = ukot::atl::SharedObject<NotificationFactoryImpl>::Create(&webNotification, client, dispatchCall, getOrigin, requestPermission);
    ATL::CComVariant param(webNotification);

    DISPPARAMS params;
    params.cArgs = 1;
    params.cNamedArgs = 0;
    params.rgvarg = &param;
    params.rgdispidNamedArgs = 0;
    hr = pWndEx->Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF, &params, nullptr, nullptr, nullptr);
    if (FAILED(hr)) {
      return;
    }
    ATL::CComPtr<IDispatch> onBeforeUnload;
    ATL::CComQIPtr<INotificationFactory> notifications = webNotification;
    hr = ukot::atl::SharedObject<OnBeforeUnloadImpl>::Create(&onBeforeUnload, notifications);
    if (FAILED(hr)) {
      return;
    }
    hr = windowEvents->addEventListener(ATL::CComBSTR(L"beforeunload"), onBeforeUnload, false);
  }
}

void STDMETHODCALLTYPE BHOImpl::OnBeforeNavigate2(/* [in] */ IDispatch* frameBrowserDisp,
  /* [in] */ VARIANT* urlVariant,
  /* [in] */ VARIANT* /*Flags*/,
  /* [in] */ VARIANT* /*TargetFrameName*/,
  /* [in] */ VARIANT* /*PostData*/,
  /* [in] */ VARIANT* /*Headers*/,
  /* [in, out] */ VARIANT_BOOL* /*Cancel*/) {
  ATL::CComQIPtr<IWebBrowser2> webBrowser = frameBrowserDisp;
  if (!webBrowser)
  {
    return;
  }

  if (webBrowser.IsEqualObject(m_data->webBrowser2) && m_data->requestPermissionWindow) {
    m_data->requestPermissionWindow->DestroyWindow();
  }

  {
    auto it = m_data->connectedWebBrowsersCache.find(webBrowser);
    if (m_data->connectedWebBrowsersCache.end() == it) {
      SERGZ_DEBUG() << L"Attach listener";
      ATL::CComObject<WebBrowserEventsListener>* listenerImpl = nullptr;
      if (FAILED(ATL::CComObject<WebBrowserEventsListener>::CreateInstance(&listenerImpl))) {
        return;
      }
      ATL::CComPtr<IUnknown> listenerRefCounterGuard(listenerImpl->GetUnknown());
      std::weak_ptr<Data> dataForCapturing = m_data;

      auto onListenerDestroy = [webBrowser, dataForCapturing] {
        if (auto data = dataForCapturing.lock()) {
          data->connectedWebBrowsersCache.erase(webBrowser);
        }
      };
      auto onReloaded = [webBrowser, dataForCapturing] {
        if (auto data = dataForCapturing.lock()) {
          InjectNotification(*webBrowser, data->client, data->dispatchCall, data->requestPermission, data->getOrigin, data->loggerFactory);
        }
      };
      if (FAILED(listenerImpl->Init(webBrowser, onListenerDestroy, onReloaded))) {
        return;
      }
      m_data->connectedWebBrowsersCache.emplace(webBrowser, listenerImpl);
    } else {
      SERGZ_DEBUG() << L"Listener is already attached";
    }
  }
}

void STDMETHODCALLTYPE BHOImpl::OnOnQuit() {
  SERGZ_DEBUG() << L"OnQuit";
  if (m_data->requestPermissionWindow) {
    m_data->requestPermissionWindow->DestroyWindow();
  };
}
