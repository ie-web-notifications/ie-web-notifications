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
#include "Notification.h"
#include <sergz.utils/DPIAwareness.h>
#include "CloseButton.h"
#include "INotificationWindow.h"

namespace ukot { namespace ie_web_notifications {
  class NotificationWindow : public ATL::CWindowImpl<NotificationWindow>, protected utils::DpiAwareness
  {
  public:
    explicit NotificationWindow(const NotificationBody& notificationBody);
    ~NotificationWindow();

    BEGIN_MSG_MAP(NotificationWindow)
      MSG_WM_CREATE(OnCreate)
      MESSAGE_HANDLER(WM_LBUTTONUP, OnClick)
      MESSAGE_HANDLER(WM_RBUTTONUP, OnSettings)
      MESSAGE_HANDLER(WM_CTLCOLORSTATIC, OnCtlColor)
    END_MSG_MAP()
    void setNotificationBody(const NotificationBody& notificationBody);
    std::function<void(bool clicked)> onCloseRequest;
    std::function<void()> onSettingsRequest;
  private:
    LRESULT OnCreate(const CREATESTRUCT* /*createStruct*/);
    LRESULT OnClick(UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*handled*/);
    LRESULT OnSettings(UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*handled*/);
    LRESULT OnCtlColor(UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*handled*/);
    
    void reloadIcon();
  private:
    NotificationBody m_notificationBody;
    WTL::CFont m_titleFont;
    WTL::CFont m_bodyFont;
    WTL::CFont m_closeBtnFont;
    // TODO: replace by something else (image)
    WTL::CStatic m_iconContainer;
    WTL::CStatic m_icon;
    ATL::CImage m_iconImg;
    WTL::CStatic m_titleCtrl;
    WTL::CStatic m_bodyCtrl;
    WTL::CBrush m_bodyBgColor;
    WTL::CBrush m_iconBgColor;
    CloseButton m_closeButton;
  };

  typedef ATL::CWinTraits<WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_TOOLWINDOW | WS_EX_TOPMOST> NotificationBorderWindowStyles;
  class NotificationBorderWindow
    : public ATL::CWindowImpl<NotificationBorderWindow, ATL::CWindow, NotificationBorderWindowStyles>
    , public INotificationWindow
    , protected utils::DpiAwareness
  {
  public:
    DECLARE_WND_CLASS_EX(/*generate class name*/nullptr, CS_DROPSHADOW, WHITE_BRUSH);
    explicit NotificationBorderWindow(const NotificationBody& notificationBody);
    ~NotificationBorderWindow();

    BEGIN_MSG_MAP(NotificationBorderWindow)
      MSG_WM_CREATE(OnCreate)
    END_MSG_MAP()

    void destroy() override {
      DestroyWindow();
    }
    void setNotificationBody(const NotificationBody& notificationBody) override {
      m_content.setNotificationBody(notificationBody);
    }
    void setOnDestroyed(const std::function<void(bool clicked)>& callback) {
      m_onDestroyedCallback = callback;
    }
    void setOnSettingsRequested(const std::function<void()>& callback) {
      m_onSettingsRequest = callback;
    }
    // moves window above the previous one on the position `value`.
    void setSlot(uint8_t value) override;
  private:
    LRESULT OnCreate(const CREATESTRUCT* createStruct);
    void OnFinalMessage(HWND) override;

    // returns {windowX, windowY} on the monitor
    POINT getWindowCoordinates();
  private:
    // m_content is used as a holder of all children and we need it to have a border.
    // It seems the most correct way to have a border to set WS_POPUPWINDOW style
    // and paint the border in WM_NCPAINT but it simply does not work here.
    NotificationWindow m_content;
    std::function<void(bool clicked)> m_onDestroyedCallback;
    bool m_clicked = false;
    std::function<void()> m_onSettingsRequest;
  };
}}