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

#include <sergz.atl/pch-common.h>
#include "Notification.h"
#include "COMNotificationWindow.h"
#include "../ie_web_notifications.toast/ie_web_notificationstoast_i.h"
#include <ie_web_notifications.utils/convertTo.h>

using namespace ukot::ie_web_notifications;
using com_client::NotificationWindowCallbackImpl;
using com_client::NotificationWindowImpl;

// uses defined `hr` of HRESULT type.
#define RETURN_ON_FAIL(expr) do {hr = (expr); if (FAILED(hr)) return hr;} while(false);
#define RETURN_VOID_ON_FAIL(expr) do {hr = (expr); assert(SUCCEEDED(hr) && #expr); if (FAILED(hr)) return;} while(false);

STDMETHODIMP NotificationWindowCallbackImpl::closed(BOOL clicked) {
  if (auto data = m_data.lock()) {
    if (!!data->onWindowDestroyed) {
      data->onWindowDestroyed(clicked != VARIANT_FALSE);
    }
  }
  return S_OK;
}

STDMETHODIMP NotificationWindowCallbackImpl::requestSettingsShowing() {
  if (auto data = m_data.lock()) {
    if (!!data->onShowSettingsRequested) {
      data->onShowSettingsRequested();
    }
  }
  return S_OK;
}

NotificationWindowImpl::NotificationWindowImpl(const std::function<void(bool clicked)>& onWindowDestroyed,
  const std::function<void()>& onShowSettingsRequested)
  : m_data(std::make_shared<NotificationWindowImpl_Data>())
{
  m_data->onWindowDestroyed = onWindowDestroyed;
  m_data->onShowSettingsRequested = onShowSettingsRequested;
}

void NotificationWindowImpl::destroy() {
  m_comWindow->destroy();
}

void NotificationWindowImpl::setNotificationBody(const NotificationBody& notificationBody) {
  m_comWindow->setNotificationBody(util::convertToBSTR(notificationBody.title),
    util::convertToBSTR(notificationBody.body), util::convertToBSTR(notificationBody.iconFilePath));
}
void NotificationWindowImpl::setSlot(uint8_t slotNumber) {
  m_comWindow->setSlot(slotNumber);
}

NotificationWindowPtr NotificationWindowImpl::create(com::INotificationWindowFactory& notificationWindowFactory,
  const NotificationBody& notificationBody, const std::function<void(bool clicked)>& onWindowDestroyed,
  const std::function<void()>& onShowSettingsRequested)
{
  auto retValue = std::make_unique<NotificationWindowImpl>(onWindowDestroyed, onShowSettingsRequested);
  HRESULT hr = S_OK;
  ATL::CComPtr<com::INotificationWindowCallback> callback;
  hr = ukot::atl::SharedObject<NotificationWindowCallbackImpl>::Create(&callback, retValue->m_data);
  if (FAILED(hr) || !callback)
    return nullptr;
  hr = notificationWindowFactory.create(ATL::CComBSTR(notificationBody.title.c_str()),
    ATL::CComBSTR(notificationBody.body.c_str()),
    ATL::CComBSTR(notificationBody.iconFilePath.c_str()), callback, &retValue->m_comWindow);
  if (FAILED(hr) || !retValue->m_comWindow) {
    return nullptr;
  }
  return NotificationWindowPtr(retValue.release());
}

namespace {
  class D2DNativeNotificationWindowFactoryImpl : public ukot::ie_web_notifications::INotificationWindowFactory {
  public:
    static std::unique_ptr<D2DNativeNotificationWindowFactoryImpl> create() {
      auto retValue = std::make_unique<D2DNativeNotificationWindowFactoryImpl>();
      HRESULT hr = retValue->m_notifcationWindowFactoryImpl.CoCreateInstance(__uuidof(com::D2DNotificationWindowFactory), nullptr, CLSCTX_INPROC_SERVER);
      if (FAILED(hr) || !retValue->m_notifcationWindowFactoryImpl) {
        return nullptr;
      }
      return retValue;
    }
    NotificationWindowPtr create(const NotificationBody& notificationBody,
      const std::function<void(bool clicked)>& onWindowDestroyed,
      const std::function<void()>& onShowSettingsRequested) override {
      return com_client::NotificationWindowImpl::create(*m_notifcationWindowFactoryImpl, notificationBody, onWindowDestroyed, onShowSettingsRequested);
    }
  private:
    ATL::CComPtr<com::INotificationWindowFactory> m_notifcationWindowFactoryImpl;
  };
}

#include <sergz.utils/SynchronizedDeque.h>
#include <thread>
#include <sergz.utils/EventWithSetter.h>
namespace {
  // Yeah, it looks very hacky, though no COM marshalling.
  class ToastNotificationWindowFactoryImpl : public ukot::ie_web_notifications::INotificationWindowFactory {
    typedef std::function<void()> Call;
  public:
    ToastNotificationWindowFactoryImpl() {
      m_thread = std::thread([this]{
        HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        while (m_internalNotExit) {
          Call call = m_calls.pop_front();
          try { // catch C++ exceptions
            call();
          } catch (...) {
            // do nothing, but the thread will be alive.
          }
        }
        m_calls.clear();
        if (SUCCEEDED(hr)) {
          CoUninitialize();
        }
      });
    }

    ~ToastNotificationWindowFactoryImpl() {
      com::INotificationWindowFactory* releaseFactoryInFreeThreadedApartment = m_notifcationWindowFactoryImpl.Detach();
      m_calls.push_front([this, releaseFactoryInFreeThreadedApartment](){
        if (releaseFactoryInFreeThreadedApartment) {
          releaseFactoryInFreeThreadedApartment->Release();
        }
        m_internalNotExit = false;
      });
      try {
        m_thread.join();
      } catch (...) {
      }
    }

    static std::unique_ptr<ToastNotificationWindowFactoryImpl> create() {
      auto retValue = std::make_unique<ToastNotificationWindowFactoryImpl>();
      ukot::utils::EventWithSetter event;
      {
        auto setter = event.getSetter();
        retValue->m_calls.push_back([setter, &retValue]{
          HRESULT hr = retValue->m_notifcationWindowFactoryImpl.CoCreateInstance(__uuidof(com::ToastNotificationWindowFactory), nullptr, CLSCTX_INPROC_SERVER);
          if (FAILED(hr) || !retValue->m_notifcationWindowFactoryImpl) {
            return;
          }
          setter->set();
        });
      }
      if (!event.wait(std::chrono::seconds(10)))
        return nullptr;
      return retValue;
    }
    NotificationWindowPtr create(const NotificationBody& notificationBody,
      const std::function<void(bool clicked)>& onWindowDestroyed,
      const std::function<void()>& onShowSettingsRequested) override
    {
      NotificationWindowPtr retValue;
      ukot::utils::EventWithSetter event;
      {
        auto setter = event.getSetter();
        m_calls.push_back([this, setter, &notificationBody, &retValue, &onWindowDestroyed, &onShowSettingsRequested]{
          if (!m_notifcationWindowFactoryImpl)
            return;
          retValue = NotificationWindowImpl::create(*m_notifcationWindowFactoryImpl, notificationBody,
            onWindowDestroyed, onShowSettingsRequested);
          setter->set();
        });
      }
      event.wait(std::chrono::seconds(10));
      return retValue;
    }
  private:
    bool m_internalNotExit = true;
    ukot::utils::SynchronizedDeque<Call> m_calls;
    std::thread m_thread;
    ATL::CComPtr<com::INotificationWindowFactory> m_notifcationWindowFactoryImpl;
  };
}

NotificationWindowFactoryPtr ukot::ie_web_notifications::createD2DWindowFactory() {
  return D2DNativeNotificationWindowFactoryImpl::create();
}

NotificationWindowFactoryPtr ukot::ie_web_notifications::createToastWindowFactory() {
  return ToastNotificationWindowFactoryImpl::create();
}
