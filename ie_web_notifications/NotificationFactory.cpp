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
#include <cassert>
#include <atlstr.h>
#include "NotificationFactory.h"
#include "NotificationPrototypeImpl.h"
#include "GModel.h"
#include <ie_web_notifications.utils/convertTo.h>
#include <ShlGuid.h>
#include "IWebBrowser2Helper.h"
#include <algorithm>

using namespace ukot::ie_web_notifications;

namespace {
  ATL::CComPtr<IWebBrowser2> getWebBrowser2(IServiceProvider* serviceProvider) {
    if (!serviceProvider)
      return nullptr;
    ATL::CComPtr<IWebBrowser2> webBrowser;
    if (FAILED(serviceProvider->QueryService(SID_SWebBrowserApp, &webBrowser)))
      return nullptr;
    return webBrowser;
  }

  // This methods transforms relative URL to absolute URL as well as URL that
  // begins with "//" to the URL with corresponding protocol.
  std::wstring handleIconURL(const std::wstring& value, IServiceProvider* serviceProvider) {
    if (value.length() > 2 && value[0] == L'/' && value[1] == L'/') {
      // if begins with //
      auto htmlLocation = getHtmlLocation(getWebBrowser2(serviceProvider));
      if (!htmlLocation) {
        return std::wstring();
      }
      ATL::CComBSTR bstrProtocol;
      std::wstring protocol;
      if (FAILED(htmlLocation->get_protocol(&bstrProtocol)) || (protocol = util::convertToWString(bstrProtocol)).empty())
        return std::wstring();
      return protocol + value;
    }
    // if [a-z]+: schema then absolute
    auto ii = value.begin();
    while (ii != value.end() && std::iswalpha(*ii))
      ++ii;
    if (ii != value.begin() && ii != value.end() && *ii == L':' && ++ii != value.end()) {
      return value;
    }

    auto htmlLocation = getHtmlLocation(getWebBrowser2(serviceProvider));
    if (!htmlLocation) {
      return std::wstring();
    }
    ATL::CComBSTR bstrProtocol;
    ATL::CComBSTR bstrHost;
    ATL::CComBSTR bstrPathName;
    if ( FAILED(htmlLocation->get_protocol(&bstrProtocol))
      || FAILED(htmlLocation->get_host(&bstrHost))
      || FAILED(htmlLocation->get_pathname(&bstrPathName)))
      return std::wstring();
    auto pathName = util::convertToWString(bstrPathName);
    auto rightSlashPos = pathName.rfind(L'/');
    if (std::wstring::npos == rightSlashPos) { // not found
      rightSlashPos = 0;
    }
    pathName = pathName.substr(0, rightSlashPos);
    bool isRelativeToRoot = !value.empty() && value[0] == L'/';
    return util::convertToWString(bstrProtocol) + L"//" + util::convertToWString(bstrHost)
      + (!isRelativeToRoot ? pathName : L"") + (!value.empty() ? L"/" + value : L"");
  }

  std::wstring getStringProperty(IDispatchEx& obj, const std::wstring& propertyName) {
    DISPID dispid;
    HRESULT hr = obj.GetDispID(util::convertToBSTR(propertyName), fdexNameCaseSensitive, &dispid);
    if (FAILED(hr)) {
      return std::wstring();
    }
    DISPPARAMS params{};
    ATL::CComVariant vtResult;
    hr = obj.InvokeEx(dispid, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &params, &vtResult, nullptr, nullptr);
    if (FAILED(hr) || VT_BSTR != V_VT(&vtResult) || !vtResult.bstrVal) {
      return std::wstring();
    }
    return util::convertToWString(vtResult.bstrVal);
  }

  void callOnJsObject(IDispatchEx& jsObject, const wchar_t* method) {
    DISPID dispid;
    HRESULT hr = jsObject.GetDispID(ATL::CComBSTR{method}, fdexNameCaseSensitive, &dispid);
    if (SUCCEEDED(hr)) {
      DISPPARAMS params{};
      hr = jsObject.InvokeEx(dispid, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &params, nullptr, nullptr, nullptr);
    }
  }

  class ATL_NO_VTABLE NotificationCloseImpl :
    public ATL::CComObjectRootEx<ATL::CComSingleThreadModel>,
    public IDispatchExEmptyImpl
  {
  public:
    NotificationCloseImpl(NotificationId notificationId, const ClientPtr& client)
      : m_notificationId(notificationId), m_client(client)
    {
    }

    DECLARE_NOT_AGGREGATABLE(NotificationCloseImpl)

    BEGIN_COM_MAP(NotificationCloseImpl)
      COM_INTERFACE_ENTRY(IDispatch)
      COM_INTERFACE_ENTRY(IDispatchEx)
    END_COM_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    HRESULT FinalConstruct() { return S_OK; }
    void FinalRelease() {}

    STDMETHOD(InvokeEx)(
      /* [annotation][in] */ _In_  DISPID id,
      /* [annotation][in] */ _In_  LCID lcid,
      /* [annotation][in] */ _In_  WORD wFlags,
      /* [annotation][in] */ _In_  DISPPARAMS *pdp,
      /* [annotation][out] */ _Out_opt_  VARIANT *pvarRes,
      /* [annotation][out] */ _Out_opt_  EXCEPINFO *pei,
      /* [annotation][unique][in] */ _In_opt_  IServiceProvider *pspCaller) override {
      if (nullptr == pdp) {
        return E_POINTER;
      }

      if (0 == id && DISPATCH_METHOD == wFlags) {
        m_client->closeNotificationAsync(m_notificationId);
        return S_OK;
      }

      return E_NOTIMPL;
    }
  private:
    NotificationId m_notificationId;
    ClientPtr m_client;
  };

  void injectCloseMethod(IDispatchEx& dst, NotificationId notificationId, const ClientPtr& client) {
    ATL::CComPtr<IDispatch> closeMethod;
    HRESULT hr = ukot::atl::SharedObject<NotificationCloseImpl>::Create(&closeMethod, notificationId, client);

    ATL::CComVariant param(closeMethod);

    DISPID dispid{};
    hr = dst.GetDispID(ATL::CComBSTR(L"close"), fdexNameEnsure, &dispid);
    if (FAILED(hr)) {
      return;
    }

    DISPPARAMS params;
    params.cArgs = 1;
    params.rgvarg = &param;
    params.cNamedArgs = 1;
    DISPID named[] = { -3 };
    params.rgdispidNamedArgs = named;
    hr = dst.Invoke(dispid, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF, &params, nullptr, nullptr, nullptr);
  }
}

NotificationFactoryImpl::NotificationFactoryImpl(const ClientPtr& client
  , const std::function<void(const std::function<void()>&, bool)>& dispatchCall
  , const std::function<std::wstring()>& getOrigin
  , const RequestPermission& requestPermission)
  : m_context{std::make_shared<Context>()}
  , m_getOrigin(getOrigin)
  , m_requestPermission{requestPermission}
{
  m_context->dispatchCall = dispatchCall;
  m_context->client = client;
}

NotificationFactoryImpl::~NotificationFactoryImpl() {
}

HRESULT NotificationFactoryImpl::FinalConstruct() {
  return ukot::atl::SharedObject<NotificationPrototypeImpl>::Create(&m_notificationPrototype);
}

void NotificationFactoryImpl::FinalRelease() {
}

STDMETHODIMP NotificationFactoryImpl::GetDispID(
  /* [in] */ __RPC__in BSTR bstrName,
  /* [in] */ DWORD grfdex,
  /* [out] */ __RPC__out DISPID *pid){
  if (nullptr == bstrName) {
    return E_POINTER;
  }
  if (nullptr == pid) {
    return E_POINTER;
  }
  if ( ((grfdex & fdexNameCaseSensitive) && wcscmp(bstrName, L"prototype") == 0)
    || ((grfdex & fdexNameCaseInsensitive) && _wcsicmp_l(bstrName, L"prototype", m_LC_ALL_C.handle()) == 0)) {
    *pid = 1;
    return S_OK;
  }
  if (((grfdex & fdexNameCaseSensitive) && wcscmp(bstrName, L"permission") == 0)
    || ((grfdex & fdexNameCaseInsensitive) && _wcsicmp_l(bstrName, L"permission", m_LC_ALL_C.handle()) == 0)) {
    *pid = 2;
    return S_OK;
  }
  if (((grfdex & fdexNameCaseSensitive) && wcscmp(bstrName, L"requestPermission") == 0)
    || ((grfdex & fdexNameCaseInsensitive) && _wcsicmp_l(bstrName, L"requestPermission", m_LC_ALL_C.handle()) == 0)) {
    *pid = 3;
    return S_OK;
  }
  return DISP_E_UNKNOWNNAME;
}

STDMETHODIMP NotificationFactoryImpl::InvokeEx(
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
  _In_opt_  IServiceProvider* pspCaller) {
  if (!m_context)
    return E_ABORT;

  if (nullptr == pdp) {
    return E_POINTER;
  }

  if (1 == id && DISPATCH_PROPERTYGET == wFlags) {
    return ATL::CComVariant{m_notificationPrototype}.Detach(pvarRes);
  } else if (2 == id  && DISPATCH_PROPERTYGET == wFlags) {
    auto permission = permissionAsString(m_context->client->getPermission(m_getOrigin()));
    return ATL::CComVariant{ util::convertToBSTR(permission) }.Detach(pvarRes);
  } else if (3 == id && (DISPATCH_METHOD & wFlags)) {
    ATL::CComQIPtr<IDispatchEx> jsCallback;
    if (pdp->cArgs > 1) {
      return DISP_E_BADPARAMCOUNT;
    }
    bool hasCallback = pdp->cArgs == 1;
    if (hasCallback) {
      if (VT_DISPATCH != V_VT(pdp->rgvarg) || !(jsCallback = pdp->rgvarg->pdispVal)) {
        return DISP_E_BADVARTYPE;
      }
    }
    // else {
    //  try to use Promise
    //  It's only an idea:
    //    if no "Promise" in window do nothing
    //    create our C++ function(resolve, reject) object (simply implement IDispatchEx)
    //    pass it into new Promise, it should immediately call our function object
    //      inside that object store resovle function
    //    assign stored resolve function to jsCallback.
    //}
    // BTW, using Promise should allow to avoid synchronous call client->getPermission.
    if (!m_requestPermission) {
      return DISP_E_EXCEPTION;
    }
    auto permission = m_context->client->getPermission(m_getOrigin());
    auto callback = [jsCallback, hasCallback](Permission value) {
      DISPPARAMS params{};
      params.cArgs = 1;
      ATL::CComVariant vtPermission{ util::convertToBSTR(permissionAsString(value)) };
      params.rgvarg = &vtPermission;
      if (!!jsCallback) {
        auto hr = jsCallback->InvokeEx(0, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &params, nullptr, nullptr, nullptr);
      }
    };
    if (Permission::default == permission) {
      std::weak_ptr<Context> ctxToCatpute = m_context;
      m_requestPermission([ctxToCatpute, callback](const std::wstring& origin, Permission value){
        // destroyed context means unloaded frame, just ignore the result for that frame.
        // The request permission window can be not closed for that frame is that frame is a nested
        // frame.
        // To support such case, one need to
        // - capture in addition a weak_ptr to client,
        // - call client->setPermissionAsync
        // - check that context is valid and call callback, however `callback` should capture
        //   a raw pointer to jsCallback and jsCallback should be stored in context.
        // TODO: do it, it's actually a bug.
        if (auto ctx = ctxToCatpute.lock())
        {
          ctx->client->setPermissionAsync(origin, value);
          // TODO:
          // Actually we should firstly send it to the client and only then
          // call the callback. For now, just do it here.
          callback(value);
        }
      });
    } else {
      callback(permission);
    }
    return S_OK;
  } else if (DISPID_VALUE == id && DISPATCH_CONSTRUCT == wFlags && pdp->cArgs > 0) {
    if (VT_DISPATCH == V_VT(pdp->rgvarg)) {
      ATL::CComQIPtr<IDispatchEx> _this{ V_DISPATCH(pdp->rgvarg) };
      if (!!_this) {
        // the first arg should be title, BSTR
        std::wstring title;
        ATL::CComQIPtr<IDispatchEx> notificationOptions;
        if (pdp->cArgs == 2) {
          if (VT_BSTR == V_VT(&pdp->rgvarg[1])) {
            title = util::convertToWString(pdp->rgvarg[1].bstrVal);
          } else {
            return DISP_E_BADVARTYPE;
          }
        } else if (pdp->cArgs == 3) {
          if (VT_BSTR == V_VT(&pdp->rgvarg[2]) && VT_DISPATCH == V_VT(&pdp->rgvarg[1])) {
            title = util::convertToWString(pdp->rgvarg[2].bstrVal);
            notificationOptions = pdp->rgvarg[1].pdispVal;
          } else {
            return DISP_E_BADVARTYPE;
          }
        } else {
          return DISP_E_BADPARAMCOUNT;
        }

        auto notificationId = sendNotification(*_this, title, notificationOptions, pspCaller);
        injectCloseMethod(*_this, notificationId, m_context->client);
      }
    }
    return S_OK;
  }
  return E_NOTIMPL;
}

STDMETHODIMP NotificationFactoryImpl::beforeUnload() {
  m_context.reset();
  return S_OK;
}

NotificationId NotificationFactoryImpl::sendNotification(IDispatchEx& jsObject, const std::wstring& title,
  const ATL::CComPtr<IDispatchEx>& notificationOptions, IServiceProvider* serviceProvider) {
  // m_context is always valid here because this method is called only from InvokeEx which tests it agains nullptr.
  assert(!!m_context && "m_context should be a valid pointer");
  ClientNotification notification;
  notification.title = title;
  if (!!notificationOptions) {
    notification.body = getStringProperty(*notificationOptions, L"body");
    notification.tag = getStringProperty(*notificationOptions, L"tag");
    notification.icon = handleIconURL(getStringProperty(*notificationOptions, L"icon"), serviceProvider);
  }
  notification.origin = m_getOrigin();

  // jsObject could be already deleted when notification.callback is called.
  // To figure it out it is stored in m_context->jsObjects and a raw pointer value is captured.
  IDispatchEx* jsObjectToCapture = &jsObject;
  m_context->jsObjects.emplace_back(&jsObject);
  std::weak_ptr<Context> ctxToCatpute = m_context;
  auto dispatchWrapper = [ctxToCatpute, jsObjectToCapture](const std::function<bool(IDispatchEx&)>& call){
    if (auto ctx = ctxToCatpute.lock()) {
      // queue the callback, which calls the notification delegate (onclick, onerror, onclose, onshow)
      // send message to the current window(this, delegate type)
      if (!!ctx->dispatchCall) {
        auto x = [ctxToCatpute, call, jsObjectToCapture]{
          if (auto ctx = ctxToCatpute.lock()) {
            auto it = std::find_if(ctx->jsObjects.begin(), ctx->jsObjects.end(), [&jsObjectToCapture](const ATL::CComPtr<IDispatchEx>& obj)->bool {
              return static_cast<IDispatchEx*>(obj) == jsObjectToCapture;
            });
            if (it != ctx->jsObjects.end() && call(**it)) {
              ctx->jsObjects.erase(it);
            }
          }
        };
        ctx->dispatchCall(x, true);
      }
    }
  };

  notification.callback = [dispatchWrapper](NotificationDestroyReason arg){
    dispatchWrapper([arg](IDispatchEx& jsObject)->bool{
      if (NotificationDestroyReason::clicked == arg) {
        callOnJsObject(jsObject, L"onclick");
      } else if (NotificationDestroyReason::closed == arg) {
        callOnJsObject(jsObject, L"onclose");
      } else if (NotificationDestroyReason::replaced == arg) {
        callOnJsObject(jsObject, L"onreplaced");
      } else /*if (NotificationDestroyReason::error == arg)*/ {
        callOnJsObject(jsObject, L"onerror");
      }
      return true;
    });
  };

  notification.error = [dispatchWrapper]{
    dispatchWrapper([](IDispatchEx& jsObject)->bool{
      callOnJsObject(jsObject, L"onerror");
      return true;
    });
  };

  notification.shown = [dispatchWrapper]{
    dispatchWrapper([](IDispatchEx& jsObject)->bool{
      callOnJsObject(jsObject, L"onshown");
      return false;
    });
  };

  return m_context->client->notifyAsync(notification);
}
