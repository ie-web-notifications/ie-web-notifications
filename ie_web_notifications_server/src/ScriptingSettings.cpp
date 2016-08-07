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

#include "ScriptingSettings.h"
#include <ie_web_notifications.utils/convertTo.h>
#include "PermissionDB.h"
#include "Settings.h"

using namespace ukot::ie_web_notifications;

namespace ukot { namespace ie_web_notifications {
  bool canUseToastNotifications();
}}

namespace {
  class ATL_NO_VTABLE ValueScriptingSettingsImpl :
    public ATL::CComObjectRootEx<ATL::CComSingleThreadModel>,
    public IDispatch
  {
  public:
    ValueScriptingSettingsImpl(const std::string& origin, Permission permission)
      : m_origin(util::convertToBSTR(origin)), m_permission(permission)
    {
    }
    DECLARE_NOT_AGGREGATABLE(ValueScriptingSettingsImpl)

    BEGIN_COM_MAP(ValueScriptingSettingsImpl)
      COM_INTERFACE_ENTRY(IDispatch)
    END_COM_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    HRESULT FinalConstruct() { return S_OK; }
    void FinalRelease() {}

    STDMETHOD(GetTypeInfoCount)(/* [out] */ __RPC__out UINT *pctinfo) override {
      if (nullptr == pctinfo) {
        return E_POINTER;
      }
      *pctinfo = 0;
      return S_OK;
    }

    STDMETHOD(GetTypeInfo)(/* [in] */ UINT iTInfo, /* [in] */ LCID lcid,
      /* [out] */ __RPC__deref_out_opt ITypeInfo **ppTInfo) override {
      if (nullptr == ppTInfo) {
        return E_POINTER;
      }
      *ppTInfo = nullptr;
      return E_NOTIMPL;
    }

    STDMETHOD(GetIDsOfNames)(_In_ REFIID riid, _In_reads_(cNames) _Deref_pre_z_ LPOLESTR* rgszNames,
      _In_range_(0, 16384) UINT cNames, LCID lcid, _Out_ DISPID* rgdispid) override {
      if (!rgszNames || !rgdispid) {
        return E_POINTER;
      }
      if (cNames != 1) {
        return E_FAIL;
      }
      if (wcscmp(*rgszNames, L"origin") == 0) {
        *rgdispid = 1;
        return S_OK;
      } else if (wcscmp(*rgszNames, L"permission") == 0) {
        *rgdispid = 2;
        return S_OK;
      }
      return DISP_E_UNKNOWNNAME;
    }

    STDMETHOD(Invoke)(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
      VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr) override {
      if (!pDispParams) {
        return E_POINTER;
      }
      if (pDispParams->cNamedArgs != 0) {
        return DISP_E_NONAMEDARGS;
      }
      if (1 == dispIdMember && DISPATCH_PROPERTYGET == wFlags) {
        if (pDispParams->cArgs != 0) {
          return DISP_E_BADPARAMCOUNT;
        }
        if (pVarResult) {
          ATL::CComVariant retValue{ m_origin };
          retValue.Detach(pVarResult);
          return S_OK;
        }
      } else if (2 == dispIdMember && DISPATCH_PROPERTYGET == wFlags) {
        if (pDispParams->cArgs != 0) {
          return DISP_E_BADPARAMCOUNT;
        }
        if (pVarResult) {
          ATL::CComVariant retValue;
          switch (m_permission) {
          case Permission::granted:
            retValue = ATL::CComBSTR(L"granted");
            break;
          case Permission::denied:
            retValue = ATL::CComBSTR(L"denied");
            break;
          default:
            ;
          }
          retValue.Detach(pVarResult);
          return S_OK;
        }
      }
      return DISP_E_MEMBERNOTFOUND;
    }
  private:
    ATL::CComBSTR m_origin;
    Permission m_permission;
  };
}

ScriptingSettingsImpl::ScriptingSettingsImpl(const std::function<void()>& onClose, Settings& settings)
  : m_settings(settings)
  , m_onClose(onClose)
{
  m_settings.permissionDB().onChanged = [this]{
    if (!!m_onDbChangedSlot) {
      DISPPARAMS params{};
      auto hr = m_onDbChangedSlot->InvokeEx(0, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &params, nullptr, nullptr, nullptr);
    }
  };
  m_settings.toastNotificationsEnabledChanged = [this]{
    if (!!m_onToastNotificationsEnabledChangedSlot) {
      DISPPARAMS params{};
      auto hr = m_onToastNotificationsEnabledChangedSlot->InvokeEx(0, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &params, nullptr, nullptr, nullptr);
    }
  };
}

ScriptingSettingsImpl::~ScriptingSettingsImpl() {
  m_settings.permissionDB().onChanged = std::function<void()>();
}

STDMETHODIMP ScriptingSettingsImpl::GetIDsOfNames(REFIID /*riid*/, LPOLESTR* rgszNames, UINT cNames, LCID /*lcid*/, _Out_ DISPID* rgdispid)
{
  if (!rgszNames || !rgdispid) {
    return E_POINTER;
  }
  if (cNames != 1) {
    return E_FAIL;
  }
  if (wcscmp(*rgszNames, L"Count") == 0) {
    *rgdispid = 1;
    return S_OK;
  }
  if (wcscmp(*rgszNames, L"item") == 0) {
    *rgdispid = 2;
    return S_OK;
  }
  if (wcscmp(*rgszNames, L"Remove") == 0) {
    *rgdispid = 3;
    return S_OK;
  }
  if (wcscmp(*rgszNames, L"ConnectOnChanged") == 0) {
    *rgdispid = 4;
    return S_OK;
  }
  if (wcscmp(*rgszNames, L"SetPermission") == 0) {
    *rgdispid = 5;
    return S_OK;
  }
  if (wcscmp(*rgszNames, L"Close") == 0) {
    *rgdispid = 6;
    return S_OK;
  }
  if (wcscmp(*rgszNames, L"isToastNotificationsSystemEnabled") == 0 && m_settings.canSupportToastNotificationsWindowFactory()) {
    *rgdispid = 7;
    return S_OK;
  }
  if (wcscmp(*rgszNames, L"ConnectOnToastNotificationsChanged") == 0) {
    *rgdispid = 8;
    return S_OK;
  }
  return DISP_E_UNKNOWNNAME;
}

STDMETHODIMP ScriptingSettingsImpl::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
  VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr) {
  if (!pDispParams) {
    return E_POINTER;
  }
  //if (pDispParams->cNamedArgs != 0) {
  //  return DISP_E_NONAMEDARGS;
  //}
  if (1 == dispIdMember && DISPATCH_PROPERTYGET == wFlags) {
    if (pDispParams->cArgs != 0) {
      return DISP_E_BADPARAMCOUNT;
    }
    if (pVarResult) {
      ATL::CComVariant retValue{ m_settings.permissionDB().count() };
      retValue.Detach(pVarResult);
      return S_OK;
    }
  }
  if (2 == dispIdMember && ((wFlags & DISPATCH_METHOD) == DISPATCH_METHOD)) {
    if (pDispParams->cArgs != 1) {
      return DISP_E_BADPARAMCOUNT;
    }
    if (pDispParams->rgvarg[0].vt != VT_I4) {
      return DISP_E_TYPEMISMATCH;
    }
    if (pVarResult) {
      ATL::CComPtr<IDispatch> value;
      auto source = m_settings.permissionDB()[pDispParams->rgvarg[0].lVal];
      HRESULT hr = ukot::atl::SharedObject<ValueScriptingSettingsImpl>::Create(&value, source.first, source.second);
      ATL::CComVariant retValue{ value };
      retValue.Detach(pVarResult);
      return S_OK;
    }
  }
  if (3 == dispIdMember && ((wFlags & DISPATCH_METHOD) == DISPATCH_METHOD)) {
    if (pDispParams->cArgs != 1) {
      return DISP_E_BADPARAMCOUNT;
    }
    if (pDispParams->rgvarg[0].vt != VT_BSTR) {
      return DISP_E_TYPEMISMATCH;
    }
    ATL::CComPtr<IDispatch> value;
    m_settings.permissionDB().remove(util::convertToString(pDispParams->rgvarg[0].bstrVal));
    return S_OK;
  }
  if (4 == dispIdMember && ((wFlags & DISPATCH_METHOD) == DISPATCH_METHOD)) {
    if (pDispParams->cArgs != 1) {
      return DISP_E_BADPARAMCOUNT;
    }
    if (pDispParams->rgvarg[0].vt != VT_DISPATCH) {
      return DISP_E_TYPEMISMATCH;
    }
    m_onDbChangedSlot = ATL::CComQIPtr<IDispatchEx>(pDispParams->rgvarg[0].pdispVal);
    return S_OK;
  }
  if (5 == dispIdMember && ((wFlags & DISPATCH_METHOD) == DISPATCH_METHOD)) {
    if (pDispParams->cArgs != 2) {
      return DISP_E_BADPARAMCOUNT;
    }
    if (pDispParams->rgvarg[0].vt != VT_BOOL || pDispParams->rgvarg[1].vt != VT_BSTR) {
      return DISP_E_TYPEMISMATCH;
    }
    auto origin = util::convertToString(pDispParams->rgvarg[1].bstrVal);
    auto permission = pDispParams->rgvarg[0].boolVal == VARIANT_FALSE ? Permission::denied : Permission::granted;
    m_settings.permissionDB().setPermission(origin, permission);
    return S_OK;
  }
  if (6 == dispIdMember && ((wFlags & DISPATCH_METHOD) == DISPATCH_METHOD)) {
    if (!!m_onClose) {
      m_onClose();
    }
    return S_OK;
  }
  if (7 == dispIdMember && DISPATCH_PROPERTYGET == wFlags) {
    if (pDispParams->cArgs != 0) {
      return DISP_E_BADPARAMCOUNT;
    }
    if (pVarResult) {
      ATL::CComVariant retValue{ m_settings.isToastNotificationsEnabled() ? VARIANT_TRUE : VARIANT_FALSE };
      retValue.Detach(pVarResult);
      return S_OK;
    }
  }
  if (7 == dispIdMember && DISPATCH_PROPERTYPUT == wFlags) {
    ATL::CComVariant value;
    UINT badArgIndex = 0;
    HRESULT hr = DispGetParam(pDispParams, DISPID_PROPERTYPUT, VT_BOOL, &value, &badArgIndex);
    if (FAILED(hr)) {
      return hr;
    }
    m_settings.setToastNotificationsEnabled(value.boolVal != VARIANT_FALSE);
    return S_OK;
  }
  if (8 == dispIdMember && ((wFlags & DISPATCH_METHOD) == DISPATCH_METHOD)) {
    ATL::CComVariant arg;
    UINT badArgIndex = 0;
    HRESULT hr = DispGetParam(pDispParams, 0, VT_DISPATCH, &arg, &badArgIndex);
    if (FAILED(hr)) {
      return hr;
    }
    m_onToastNotificationsEnabledChangedSlot = ATL::CComQIPtr<IDispatchEx>(arg.pdispVal);
    return S_OK;
  }
  return DISP_E_MEMBERNOTFOUND;
}