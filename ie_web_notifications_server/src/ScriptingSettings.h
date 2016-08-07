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
#include <sergz.atl/pch-common.h>
#include <sergz.atl/atl.h>
#include <functional>
#include <ExDispid.h>

namespace ukot { namespace ie_web_notifications {
  class Settings;
  class ATL_NO_VTABLE ScriptingSettingsImpl :
    public ATL::CComObjectRootEx<ATL::CComSingleThreadModel>,
    public IDispatch
  {
  public:
    ScriptingSettingsImpl(const std::function<void()>& onClose, Settings& settings);
    ~ScriptingSettingsImpl();
    DECLARE_NOT_AGGREGATABLE(ScriptingSettingsImpl)

    BEGIN_COM_MAP(ScriptingSettingsImpl)
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
      _In_range_(0, 16384) UINT cNames, LCID lcid, _Out_ DISPID* rgdispid) override;

    STDMETHOD(Invoke)(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
      VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr) override;
  private:
    Settings& m_settings;
    std::function<void()> m_onClose;
    ATL::CComPtr<IDispatchEx> m_onDbChangedSlot;
    ATL::CComPtr<IDispatchEx> m_onToastNotificationsEnabledChangedSlot;
  };
}}