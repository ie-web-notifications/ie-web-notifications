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
#include <algorithm>
using std::max;
using std::min;
#include <atlimage.h>
// wtl headers
#include <atlapp.h>
#include <atlframe.h>
#include <atlmisc.h>
#include <atlctrls.h>
#include <atlcrack.h>
#include <functional>
#include <vector>
#include <sergz.utils/DPIAwareness.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include "D2DDeviceIndependentResources.h"
#include <ie_web_notifications.utils/NotificationBody.h>

namespace ukot { namespace ie_web_notifications {

  // Although the background is cleared using Direct2D there is no dummy
  // handling of WM_ERASEBKGND because it has a weird effect when the window is
  // drawn. Initially some parts of the window are transparent and it's filled
  // only afterwards. Unfortunately this effect is visible. However if it cause
  // some flickering it can be removed later but should be tested.

  class D2DNotificationWindow : public ATL::CWindowImpl<D2DNotificationWindow>, protected utils::DpiAwareness
  {
  public:
    explicit D2DNotificationWindow(const D2DDeviceIndependentResources& d2d, const NotificationBody& notificationBody);
    ~D2DNotificationWindow();

    BEGIN_MSG_MAP(D2DNotificationWindow)
      MSG_WM_CREATE(OnCreate)
      MSG_WM_SIZE(OnResize)
      MSG_WM_PAINT(OnPaint)
      MSG_WM_MOUSEMOVE(OnMouseMove)
      MSG_WM_MOUSELEAVE(OnMouseLeave)
      MSG_WM_LBUTTONUP(OnLMouseBtnUp) /* on click */
      MSG_WM_TIMER(OnTimer)
      MSG_WM_DESTROY(OnDestroy)
      MESSAGE_HANDLER(WM_RBUTTONUP, OnSettings)
    END_MSG_MAP()

    void setNotificationBody(const NotificationBody& notificationBody);
    std::function<void(bool clicked)> onCloseRequest;
    std::function<void()> onSettingsRequest;
  private:
    LRESULT OnCreate(const CREATESTRUCT* /*createStruct*/);
    void OnResize(WPARAM /*wParam*/, const WTL::CSize& size);
    void OnPaint(HDC);
    void OnMouseMove(WPARAM, const WTL::CPoint& point);
    void OnMouseLeave();
    void OnLMouseBtnUp(WPARAM /*wParam*/, const WTL::CPoint& point);
    void OnTimer(UINT_PTR timerID);
    void OnDestroy();
    LRESULT OnSettings(UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*handled*/);

    HRESULT createDeviceResources();
    void discardDeviceResources();

    void reloadIcon();
  private:
    D2DDeviceIndependentResources m_d2d;
    ATL::CComPtr<IDWriteTextFormat> m_titleTextFormat;
    NotificationBody m_notificationBody;
    // Sotre base64 decoded data because I'm not 100% sure that we may safely 
    // release the buffer right after the call of `IWICStream::InitializeFromMemory`.
    // TODO: look into a better solution because it' still can cause troubles or find a proof that
    // it does always work.
    std::vector<uint8_t> m_imageData;
    ATL::CComPtr<ID2D1HwndRenderTarget> m_renderTarget;
    // background for the icon
    ATL::CComPtr<ID2D1SolidColorBrush> m_greyBrush;
    ATL::CComPtr<ID2D1SolidColorBrush> m_blackBrush;
    ATL::CComPtr<ID2D1SolidColorBrush> m_whiteBrush;
    ATL::CComPtr<ID2D1SolidColorBrush> m_redBrush;
    ATL::CComPtr<ID2D1Bitmap> m_d2Bitmap;
    ATL::CComPtr<IWICBitmapSource> m_wicBitmap;
    D2D1_RECT_F m_closeButtonRect;
    bool m_closeButtonIsMouseOver = false;
    bool m_isTrackingMouseEvents = false;
    UINT_PTR m_paintTimer = NULL;
  };

  typedef ATL::CWinTraits<WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_TOOLWINDOW | WS_EX_TOPMOST> NotificationBorderWindowStyles;
  class D2DNotificationBorderWindow
    : public ATL::CWindowImpl<D2DNotificationBorderWindow, ATL::CWindow, NotificationBorderWindowStyles>
    , protected utils::DpiAwareness
  {
  public:
    DECLARE_WND_CLASS_EX(/*generate class name*/nullptr, CS_DROPSHADOW, WHITE_BRUSH);
    explicit D2DNotificationBorderWindow(const D2DDeviceIndependentResources& d2d, const NotificationBody& notificationBody);
    ~D2DNotificationBorderWindow();

    BEGIN_MSG_MAP(D2DNotificationBorderWindow)
      MSG_WM_CREATE(OnCreate)
      MSG_WM_SIZE(OnResize)
      MSG_WM_PAINT(OnPaint)
    END_MSG_MAP()

    void destroy() {
      DestroyWindow();
    }
    void setNotificationBody(const NotificationBody& notificationBody) {
      m_content.setNotificationBody(notificationBody);
    }
    void setOnDestroyed(const std::function<void(bool clicked)>& callback) {
      m_onDestroyedCallback = callback;
    }
    void setOnSettingsRequested(const std::function<void()>& callback) {
      m_onSettingsRequest = callback;
    }
    // moves window above the previous one on the position `value`.
    void setSlot(uint8_t value);
  private:
    LRESULT OnCreate(const CREATESTRUCT* createStruct);
    void OnResize(WPARAM /*wParam*/, const WTL::CSize& size);
    void OnPaint(HDC);

    void OnFinalMessage(HWND) override;

    HRESULT createDeviceResources();
    void discardDeviceResources();

    // returns {windowX, windowY} on the monitor
    POINT getWindowCoordinates();
  private:
    D2DDeviceIndependentResources m_d2d;
    ATL::CComPtr<ID2D1HwndRenderTarget> m_renderTarget;
    // m_content is used as a holder of all children and we need it to have a border.
    // It seems the most correct way to have a border to set WS_POPUPWINDOW style
    // and paint the border in WM_NCPAINT but it simply does not work here.
    D2DNotificationWindow m_content;
    std::function<void(bool clicked)> m_onDestroyedCallback;
    bool m_clicked = false;
    std::function<void()> m_onSettingsRequest;
  };
}}