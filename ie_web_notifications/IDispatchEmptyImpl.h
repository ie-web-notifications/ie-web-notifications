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
#include <ExDispid.h>

namespace ukot { namespace ie_web_notifications {
  class ATL_NO_VTABLE IDispatchExEmptyImpl : public IDispatchEx
  {
  public:
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
      if (nullptr == rgszNames) {
        return E_POINTER;
      }
      if (nullptr == rgdispid) {
        return E_POINTER;
      }
      *rgdispid = DISPID_UNKNOWN;
      return DISP_E_UNKNOWNNAME;
    }

    STDMETHOD(Invoke)(DISPID dispIdMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
      VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr) override {
      return E_NOTIMPL;
    }

    STDMETHOD(GetDispID)(/* [in] */ __RPC__in BSTR bstrName, /* [in] */ DWORD grfdex,
      /* [out] */ __RPC__out DISPID *pid) override {
      if (nullptr == bstrName) {
        return E_POINTER;
      }
      if (nullptr == pid) {
        return E_POINTER;
      }
      return DISP_E_UNKNOWNNAME;
    }

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
      return DISP_E_MEMBERNOTFOUND;
    }

    STDMETHOD(DeleteMemberByName)(/* [in] */ __RPC__in BSTR bstrName, /* [in] */ DWORD grfdex) override {
      return E_NOTIMPL;
    }

    STDMETHOD(DeleteMemberByDispID)(/* [in] */ DISPID id) override {
      return E_NOTIMPL;
    }

    STDMETHOD(GetMemberProperties)(/* [in] */ DISPID id, /* [in] */ DWORD grfdexFetch,
      /* [out] */ __RPC__out DWORD *pgrfdex) override {
      return E_NOTIMPL;
    }

    STDMETHOD(GetMemberName)(/* [in] */ DISPID id, /* [out] */ __RPC__deref_out_opt BSTR *pbstrName) override {
      return E_NOTIMPL;
    }

    STDMETHOD(GetNextDispID)(/* [in] */ DWORD grfdex, /* [in] */ DISPID id, /* [out] */ __RPC__out DISPID *pid) override {
      if (nullptr == pid) {
        return E_POINTER;
      }
      return E_NOTIMPL;
    }

    STDMETHOD(GetNameSpaceParent)(/* [out] */ __RPC__deref_out_opt IUnknown **ppunk) override {
      return E_NOTIMPL;
    }
  };
}}