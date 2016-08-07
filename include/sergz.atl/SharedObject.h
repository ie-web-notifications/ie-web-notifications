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
#include <Unknwn.h>
#include <cassert>
#include <atlbase.h>
#include <atlcom.h>
#include <utility>

namespace ukot { namespace atl { namespace detail {
  template<class T>
  bool IsOutputPointerValid(T* p) {
    // Unfortunately the pointers from Microsoft often are not initialized and it triggers here.
    bool valid = nullptr != p/* && nullptr == *p*/;
    assert(valid && "Output pointer cannot be nullptr and must point to nullptr");
    return valid;
  }

  template<class T>
  HRESULT AssignToOutputPointer(T** pp, ATL::CComPtr<T>& p) {
    if ( !IsOutputPointerValid(pp) ) {
      return E_POINTER;
    }

    (*pp) = p;
    if (0 != (*pp)) {
      (*pp)->AddRef();
    }
    return S_OK;
  }

  template <class T, class I>
  static HRESULT InitializeHelper(ATL::CComPtr<I>& comPtr, __out I** object, __out T** objectImpl = nullptr) {
    static_assert(std::is_base_of<I, T>::value, "Template arg T must derive from I");

    if (!detail::IsOutputPointerValid(object)) {
      return E_POINTER;
    }

    if (nullptr != objectImpl && nullptr != *objectImpl) {
      return E_POINTER;
    }

    HRESULT hr = S_OK;
    hr = static_cast<T&>(*comPtr)._AtlInitialConstruct();
    if (SUCCEEDED(hr)) {
      hr = static_cast<T&>(*comPtr).FinalConstruct();
    }
    if (SUCCEEDED(hr)) {
      hr = static_cast<T&>(*comPtr)._AtlFinalConstruct();
    }
    if (SUCCEEDED(hr)) {
      hr = detail::AssignToOutputPointer(object, comPtr);
      if (nullptr != objectImpl) {
        *objectImpl = &static_cast<T&>(*comPtr);
      }
    }

    assert(SUCCEEDED(hr) && "ukot::atl::detail::InitializeHelper failed");

    return hr;
  }
} // namespace detail


  // The idea of this class is that it allows to construct the COM classes and pass parameters to
  // the constructor of it.
  // This class implements AddRef, Relase and QueryInterface.
  // However, QueryInterface requires a helper function: T::QueryInterfaceHelper(IID const&, void**)
  // that must be implemented by all classes using the COMObject
  // !!! WARNING: Don't call AddRef() or Release() in the current object's constructor. It's
  // not designed for that as well as the usual ATL case. Check that none of ctr arguments does it as well.
  // These classes rely a lot on ATL and it requires T to inherit from ATL::CComObjectRootEx
  template <class T>
  class SharedObject: public T {
    static_assert(std::is_base_of<ATL::CComObjectRootEx<typename T::_ThreadModel>, T>::value,
      "Template arg T must derive from ATL::CComObjectRootEx");
    ATL::ModuleLockHelper m_scopedAtlModuleLock;
  public:
    template <typename I>
    static HRESULT Create(__out I** object, __out T** objectImpl = nullptr) {
      static_assert(std::is_base_of<I, T>::value, "Template arg T must derive from I");

      ATL::CComPtr<I> comPtr = static_cast<T*>(new (std::nothrow) SharedObject<T>());
      if (!comPtr) {
        return E_FAIL;
      }

      return detail::InitializeHelper<T>(comPtr, object, objectImpl);
    }

    template <typename I, typename... Args>
    static HRESULT Create(__out I** object, Args&&... args) {
      static_assert(std::is_base_of<I, T>::value, "Template arg T must derive from I");

      ATL::CComPtr<I> comPtr = static_cast<T*>(new (std::nothrow) SharedObject<T>(std::forward<Args>(args)...));
      if (!comPtr) {
          return E_FAIL;
      }

      return detail::InitializeHelper<T>(comPtr, object, nullptr);
    }

  private:
    template <typename... Args>
    SharedObject(Args&&... args)
      : T(std::forward<Args>(args)...)
    {
    }

    ~SharedObject() {
      m_dwRef = -(LONG_MAX / 2);
      __if_exists(T::FinalRelease) {
        this->FinalRelease();
      }
    }

    HRESULT __stdcall QueryInterface(const IID &iid, void **object) {
      if (nullptr == object) {
        return E_POINTER;
      }

      *object = nullptr;
      return this->_InternalQueryInterface(iid, object);
    }

    STDMETHOD_(ULONG, AddRef)() override {
      return InternalAddRef();
    }
    STDMETHOD_(ULONG, Release)() override {
      ULONG l = InternalRelease();
      if (l == 0) {
        ATL::ModuleLockHelper lock;
        delete this;
      }
      return l;
    }
  };

namespace detail {
  template <class Base> //Base must be derived from CComObjectRoot
  class ContainedObject : public Base {
    static_assert(std::is_base_of<ATL::CComObjectRootEx<typename Base::_ThreadModel>, Base>::value,
      "Template arg Base must derive from ATL::CComObjectRootEx");
  public:
    template<typename... Args>
    ContainedObject(IUnknown* outerUnk, Args&&... args)
      : Base(std::forward<Args>(args)...)
    {
      m_pOuterUnknown = outerUnk;
      assert(nullptr != m_pOuterUnknown && "outer must not be nullptr");
    }
#if defined(_ATL_DEBUG_INTERFACES) && !defined(_ATL_STATIC_LIB_IMPL)
    virtual ~ContainedObject()
    {
      ATL::_AtlDebugInterfacesModule.DeleteNonAddRefThunk(_GetRawUnknown());
      ATL::_AtlDebugInterfacesModule.DeleteNonAddRefThunk(m_pOuterUnknown);
    }
#endif

    STDMETHOD_(ULONG, AddRef)() throw() override {
      return OuterAddRef();
    }
    STDMETHOD_(ULONG, Release)() throw() override {
      return OuterRelease();
    }
    STDMETHOD(QueryInterface)(const IID& iid, void** ppvObject) throw() {
      return OuterQueryInterface(iid, ppvObject);
    }
    //GetControllingUnknown may be virtual if the Base class has declared
    //DECLARE_GET_CONTROLLING_UNKNOWN()
    IUnknown* GetControllingUnknown() throw() {
#if defined(_ATL_DEBUG_INTERFACES) && !defined(_ATL_STATIC_LIB_IMPL)
      IUnknown* p;
      _AtlDebugInterfacesModule.AddNonAddRefThunk(m_pOuterUnknown, _T("ukot::atl::detail::ContainedObject"), &p);
      return p;
#else
      return m_pOuterUnknown;
#endif
    }
  };
}

  template<class T>
  class SharedAggObject : public IUnknown, public ATL::CComObjectRootEx<typename T::_ThreadModel::ThreadModelNoCS> {
    ATL::ModuleLockHelper m_scopedAtlModuleLock;
  public:
    template <typename... Args>
    static HRESULT Create(IUnknown* outerUnk, __out IUnknown** nondelegatingIUnknown, Args&&... args) {
      if (nullptr == outerUnk) {
        return E_POINTER;
      }
      ATL::CComPtr<IUnknown> comPtr = new (std::nothrow) SharedAggObject<T>(outerUnk, std::forward<Args>(args)...);
      if (!comPtr) {
        return E_FAIL;
      }

      return detail::InitializeHelper<SharedAggObject<T>>(comPtr, nondelegatingIUnknown, nullptr);
    }
    HRESULT _AtlInitialConstruct() {
      HRESULT hr = m_contained._AtlInitialConstruct();
      if (SUCCEEDED(hr)) {
        hr = CComObjectRootEx<typename T::_ThreadModel::ThreadModelNoCS>::_AtlInitialConstruct();
      }
      return hr;
    }
    HRESULT FinalConstruct() {
      CComObjectRootEx<T::_ThreadModel::ThreadModelNoCS>::FinalConstruct();
      return m_contained.FinalConstruct();
    }
    void FinalRelease() {
      m_contained.FinalRelease();
      CComObjectRootEx<T::_ThreadModel::ThreadModelNoCS>::FinalRelease();
    }
    virtual ~SharedAggObject() {
      m_dwRef = -(LONG_MAX / 2);
      FinalRelease();
#if defined(_ATL_DEBUG_INTERFACES) && !defined(_ATL_STATIC_LIB_IMPL)
      ATL::_AtlDebugInterfacesModule.DeleteNonAddRefThunk(this);
#endif
    }
  private:
    template <typename... Args>
    SharedAggObject(IUnknown* outerUnk, Args&&... args)
      : m_contained(outerUnk, std::forward<Args>(args)...)
    {
    }
    STDMETHOD_(ULONG, AddRef)() override {
      return InternalAddRef();
    }
    STDMETHOD_(ULONG, Release)() {
      ULONG l = InternalRelease();
      if (0 == l) {
        ATL::ModuleLockHelper lock;
        delete this;
      }
      return l;
    }
    STDMETHOD(QueryInterface)(REFIID iid, void** ppvObject) {
      if (!detail::IsOutputPointerValid(ppvObject)) {
        return E_POINTER;
      }
      *ppvObject = nullptr;

      HRESULT hr = S_OK;
      if (ATL::InlineIsEqualUnknown(iid)) {
        *ppvObject = static_cast<void*>(static_cast<IUnknown*>(this));
        AddRef();
#if defined(_ATL_DEBUG_INTERFACES) && !defined(_ATL_STATIC_LIB_IMPL)
        ATL::_AtlDebugInterfacesModule.AddThunk(reinterpret_cast<IUnknown**>(ppvObject),
          reinterpret_cast<LPCTSTR>(T::_GetEntries()[-1].dw), iid);
#endif // _ATL_DEBUG_INTERFACES
      } else {
        hr = m_contained._InternalQueryInterface(iid, ppvObject);
      }
      return hr;
    }
    detail::ContainedObject<T> m_contained;
  };
}} // namespace ukot::atl
