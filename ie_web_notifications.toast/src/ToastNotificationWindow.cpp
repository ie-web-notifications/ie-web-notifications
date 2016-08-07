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
#include "ToastNotificationWindow.h"
#include <vector>

using namespace ukot::ie_web_notifications;
using Microsoft::WRL::ComPtr;
using ABI::Windows::UI::Notifications::IToastNotificationManagerStatics;
using ABI::Windows::UI::Notifications::ToastTemplateType_ToastImageAndText02;
using ABI::Windows::UI::Notifications::ToastTemplateType_ToastText02;
using ABI::Windows::Data::Xml::Dom::IXmlDocument;
using ABI::Windows::Data::Xml::Dom::IXmlNodeList;
using ABI::Windows::Data::Xml::Dom::IXmlNode;
using ABI::Windows::Data::Xml::Dom::IXmlNamedNodeMap;
using ABI::Windows::Data::Xml::Dom::IXmlText;

namespace {
  const wchar_t kAppId[] = L"ukot.ie_web_notifications";

  typedef ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::UI::Notifications::ToastNotification*, ::IInspectable *> ToastNotificationActivatedEventHandler;
  typedef ABI::Windows::Foundation::ITypedEventHandler<ABI::Windows::UI::Notifications::ToastNotification*, ABI::Windows::UI::Notifications::ToastDismissedEventArgs*> ToastNotificationDismissedEventHandler;

  class ToastEventHandler :
    public ATL::CComObjectRootEx<ATL::CComMultiThreadModel>,
    public Microsoft::WRL::Implements<ToastNotificationActivatedEventHandler, ToastNotificationDismissedEventHandler>
  {
  public:
    ToastEventHandler::ToastEventHandler(com::INotificationWindowCallback* callback)
      : m_callback(callback)
    {

    }
    ~ToastEventHandler(){}
    DECLARE_NOT_AGGREGATABLE(ToastEventHandler)

    BEGIN_COM_MAP(ToastEventHandler)
      COM_INTERFACE_ENTRY(ToastNotificationActivatedEventHandler)
      COM_INTERFACE_ENTRY(ToastNotificationDismissedEventHandler)
    END_COM_MAP()

    DECLARE_PROTECT_FINAL_CONSTRUCT()

    HRESULT FinalConstruct(){ return S_OK; }
    void FinalRelease(){}

    // DesktopToastActivatedEventHandler 
    STDMETHOD(Invoke)(_In_ ABI::Windows::UI::Notifications::IToastNotification *sender, _In_ IInspectable* args) override {
      if (m_callback) {
        m_callback->closed(VARIANT_TRUE);
      }
      return S_OK;
    }

    // DesktopToastDismissedEventHandler
    STDMETHOD(Invoke)(_In_ ABI::Windows::UI::Notifications::IToastNotification *sender, _In_ ABI::Windows::UI::Notifications::IToastDismissedEventArgs *e) override {
      if (m_callback) {
        m_callback->closed(VARIANT_FALSE);
      }
      return S_OK;
    }

  private:
    ATL::CComPtr<com::INotificationWindowCallback> m_callback;
  };

  // Create and display the toast
  HRESULT createToast(IToastNotificationManagerStatics& toastManager, IXmlDocument& xml, com::INotificationWindowCallback* callback)
  {
    using ABI::Windows::UI::Notifications::IToastNotifier;
    using ABI::Windows::UI::Notifications::IToastNotification;
    using ABI::Windows::UI::Notifications::IToastNotificationFactory;
    ComPtr<ABI::Windows::UI::Notifications::IToastNotifier> notifier;
    HRESULT hr = toastManager.CreateToastNotifierWithId(StringReferenceWrapper(kAppId).Get(), &notifier);
    if (FAILED(hr)) {
      return hr;
    }
    ComPtr<IToastNotificationFactory> factory;
    using Windows::Foundation::GetActivationFactory;
    hr = GetActivationFactory(StringReferenceWrapper(RuntimeClass_Windows_UI_Notifications_ToastNotification).Get(), &factory);
    if (FAILED(hr)) {
      return hr;
    }
    ComPtr<IToastNotification> toast;
    hr = factory->CreateToastNotification(&xml, &toast);
    if (FAILED(hr)) {
      return hr;
    }
    // Register the event handlers
    EventRegistrationToken activatedToken, dismissedToken, failedToken;
    ATL::CComPtr<ToastNotificationDismissedEventHandler> dismissedEventHandler;
    hr = ukot::atl::SharedObject<ToastEventHandler>::Create(&dismissedEventHandler, callback);
    if (FAILED(hr)) {
      return hr;
    }
    hr = toast->add_Dismissed(dismissedEventHandler, &dismissedToken);
    if (FAILED(hr)) {
      return hr;
    }
    ATL::CComQIPtr<ToastNotificationActivatedEventHandler> activatedEventHandler = dismissedEventHandler;
    if (!activatedEventHandler) {
      return E_ABORT;
    }
    hr = toast->add_Activated(activatedEventHandler, &activatedToken);
    if (FAILED(hr)) {
      return hr;
    }
    //X        hr = toast->add_Failed(eventHandler.Get(), &failedToken);
    hr = notifier->Show(toast.Get());
    return hr;
  }

  HRESULT setNodeValueString(HSTRING inputString, IXmlNode& node, IXmlDocument& xml) {
    ComPtr<IXmlText> inputText;
    HRESULT hr = xml.CreateTextNode(inputString, &inputText);
    if (FAILED(hr)) {
      return hr;
    }
    ComPtr<IXmlNode> inputTextNode;
    hr = inputText.As(&inputTextNode);
    if (FAILED(hr)) {
      return hr;
    }
    ComPtr<IXmlNode> pAppendedChild;
    hr = node.AppendChild(inputTextNode.Get(), &pAppendedChild);
    return hr;
  }

  // Set the values of each of the text nodes
  HRESULT setTextValues(const std::vector<std::wstring>& texts, IXmlDocument& toastXml)
  {
    HRESULT hr = !texts.empty() ? S_OK : E_INVALIDARG;
    if (FAILED(hr)) {
      return hr;
    }

    ComPtr<IXmlNodeList> nodeList;
    hr = toastXml.GetElementsByTagName(StringReferenceWrapper(L"text").Get(), &nodeList);
    if (FAILED(hr)) {
      return hr;
    }
    UINT32 nodeListLength;
    hr = nodeList->get_Length(&nodeListLength);
    if (FAILED(hr)) {
      return hr;
    }

    hr = texts.size() <= nodeListLength ? S_OK : E_INVALIDARG;
    if (FAILED(hr)) {
      return hr;
    }
    for (size_t i = 0; i < texts.size(); ++i) {
      ComPtr<IXmlNode> textNode;
      hr = nodeList->Item(i, &textNode);
      if (FAILED(hr))
      {
        return hr;
      }
      hr = setNodeValueString(StringReferenceWrapper(texts[i]).Get(), *textNode.Get(), toastXml);
      if (FAILED(hr))
      {
        return hr;
      }
    }
    return hr;
  }
}

ToastNotificationWindowImpl::ToastNotificationWindowImpl(const NotificationBody& notificationBody,
  com::INotificationWindowCallback* callback)
  : m_notificationBody(notificationBody)
  , m_notificationWindowCallback(callback)
{

}

ToastNotificationWindowImpl::~ToastNotificationWindowImpl() {

}

HRESULT ToastNotificationWindowImpl::FinalConstruct() {
  ComPtr<IToastNotificationManagerStatics> toastStatics;
  HRESULT hr = Windows::Foundation::GetActivationFactory(StringReferenceWrapper(RuntimeClass_Windows_UI_Notifications_ToastNotificationManager).Get(), &toastStatics);

  if (FAILED(hr)) {
    return hr;
  }
  ComPtr<IXmlDocument> toastXml;
  hr = createToastXml(*toastStatics.Get(), &toastXml);
  if (SUCCEEDED(hr))
  {
    hr = createToast(*toastStatics.Get(), *toastXml.Get(), m_notificationWindowCallback);
  }

  return hr;
}

void ToastNotificationWindowImpl::FinalRelease() {
}

HRESULT ToastNotificationWindowImpl::createToastXml(IToastNotificationManagerStatics& toastManager, _Outptr_ IXmlDocument** inputXml) {
  if (!inputXml || *inputXml != nullptr) {
    return E_POINTER;
  }
  bool shouldTryWithImage = !m_notificationBody.iconFilePath.empty();
  HRESULT hr = toastManager.GetTemplateContent((shouldTryWithImage ? ToastTemplateType_ToastImageAndText02 : ToastTemplateType_ToastText02), inputXml);
  if (FAILED(hr) || !*inputXml) {
    return hr;
  }

  if (shouldTryWithImage)
    if (FAILED(hr = setImageSrc(**inputXml)))
      return hr;

  std::vector<std::wstring> texts{ m_notificationBody.title, m_notificationBody.body };
  hr = setTextValues(texts, **inputXml);
  return hr;
}

HRESULT ToastNotificationWindowImpl::setImageSrc(IXmlDocument& toastXml)
{
  ComPtr<IXmlNodeList> nodeList;
  HRESULT hr = toastXml.GetElementsByTagName(StringReferenceWrapper(L"image").Get(), &nodeList);
  if (FAILED(hr)) {
    return hr;
  }

  ComPtr<IXmlNode> imageNode;
  hr = nodeList->Item(0, &imageNode);
  if (FAILED(hr)) {
    return hr;
  }
  ComPtr<IXmlNamedNodeMap> attributes;

  hr = imageNode->get_Attributes(&attributes);
  if (FAILED(hr)) {
    return hr;
  }
  ComPtr<IXmlNode> srcAttribute;
  hr = attributes->GetNamedItem(StringReferenceWrapper(L"src").Get(), &srcAttribute);
  if (FAILED(hr)) {
    return hr;
  }
  auto iconPath = L"file:///" + m_notificationBody.iconFilePath;
  hr = setNodeValueString(StringReferenceWrapper(iconPath).Get(), *srcAttribute.Get(), toastXml);
  return hr;
}

STDMETHODIMP ToastNotificationWindowImpl::InterfaceSupportsErrorInfo(REFIID riid)
{
  static const IID* const arr[] = 
  {
    &__uuidof(com::INotificationWindow)
  };

  for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
  {
    if (InlineIsEqualGUID(*arr[i],riid))
      return S_OK;
  }
  return S_FALSE;
}
