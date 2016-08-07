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

#include "NotificationWindow.h"
#include <cstdint>
#include <Winuser.h>

using namespace ukot::ie_web_notifications;

namespace {
  const uint32_t kWindowWidth = 350;
  const uint32_t kWindowHeight = 70;
  const uint32_t kWindowPaddingRight = 20;
  const uint32_t kWindowPaddingBottom = 40;
  const uint32_t kTitleHeight = 30;
  const uint32_t kCloseButtonWidth = 20;
  const uint32_t kDesiredIconSize = 40;
}

NotificationWindow::NotificationWindow(const NotificationBody& notificationBody)
  : m_notificationBody(notificationBody)
{
}

NotificationWindow::~NotificationWindow(){
}

LRESULT NotificationWindow::OnCreate(const CREATESTRUCT* /*createStruct*/) {
  {
    WTL::CDC hdc = GetDC();
    m_dpi= hdc.GetDeviceCaps(LOGPIXELSX);
  }
  m_iconBgColor.CreateSolidBrush(RGB(240, 240, 240));
  auto iconWidth = kWindowHeight;
  m_iconContainer.Create(m_hWnd,
    WTL::CRect(POINT{0, 0}, DPIAware(SIZE{kWindowHeight, kWindowHeight})),
    nullptr, WS_CHILD | WS_VISIBLE, 0, 103);
  m_icon.Create(m_hWnd,
    DPIAware(WTL::CRect(POINT{ (iconWidth - kDesiredIconSize) / 2, (iconWidth - kDesiredIconSize) / 2 }, SIZE{ kDesiredIconSize, kDesiredIconSize })),
    nullptr, WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE);
  reloadIcon();

  {
    LOGFONT logFont = { 0 };
    logFont.lfQuality = CLEARTYPE_QUALITY;
    logFont.lfHeight = 100;
    logFont.lfWeight = FW_MEDIUM;
    SecureHelper::strncpy_x(logFont.lfFaceName, _countof(logFont.lfFaceName), L"Segoe UI", _TRUNCATE);
    m_titleFont.CreatePointFontIndirect(&logFont);
  }
  uint32_t titleMargingTop = 10;
  uint32_t titleMargingLeft = 10;
  m_titleCtrl.Create(m_hWnd,
    DPIAware(WTL::CRect{ POINT{ iconWidth + titleMargingLeft, titleMargingTop },
                        SIZE{kWindowWidth - kCloseButtonWidth - iconWidth - titleMargingLeft,
                        kTitleHeight - titleMargingTop } }),
    m_notificationBody.title.c_str(), WS_CHILD | WS_VISIBLE, 0, 101);
  m_titleCtrl.SetFont(m_titleFont);

  uint32_t bodyMargingTop = 0;
  uint32_t bodyMargingLeft = 5;
  m_bodyCtrl.Create(m_hWnd,
    DPIAware(WTL::CRect{ POINT{ iconWidth + bodyMargingLeft, kTitleHeight + bodyMargingTop },
                        SIZE{kWindowWidth - iconWidth - bodyMargingLeft,
                             kWindowHeight - kTitleHeight - bodyMargingTop}}),
    m_notificationBody.body.c_str(), WS_CHILD | WS_VISIBLE, 0, 102);
  {
    LOGFONT logFont = { 0 };
    logFont.lfQuality = CLEARTYPE_QUALITY;
    logFont.lfHeight = 88;
    SecureHelper::strncpy_x(logFont.lfFaceName, _countof(logFont.lfFaceName), L"Segoe UI", _TRUNCATE);
    m_bodyFont.CreatePointFontIndirect(&logFont);
  }
  m_bodyCtrl.SetFont(m_bodyFont);
  m_bodyBgColor.CreateSolidBrush(RGB(255, 255, 255));

  uint32_t buttonMargin = 5;
  m_closeButton.Create(m_hWnd,
    DPIAware(WTL::CRect{ POINT{ kWindowWidth - kCloseButtonWidth, 0 },
                        SIZE{kCloseButtonWidth - buttonMargin, kCloseButtonWidth - buttonMargin}}),
    L"x");
  {
    LOGFONT logFont = { 0 };
    logFont.lfCharSet = DEFAULT_CHARSET;
    logFont.lfHeight = 60;
    SecureHelper::strncpy_x(logFont.lfFaceName, _countof(logFont.lfFaceName), L"Consolas", _TRUNCATE);
    m_closeBtnFont.CreatePointFontIndirect(&logFont);
  }
  m_closeButton.SetFont(m_closeBtnFont);
  m_closeButton.clicked = [this]{
    onCloseRequest(false);
  };
  SetMsgHandled(false);
  return 0;
}

LRESULT NotificationWindow::OnClick(UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*handled*/) {
  onCloseRequest(true);
  return 0;
}

LRESULT NotificationWindow::OnSettings(UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*handled*/) {
  if (!!onSettingsRequest) {
    onSettingsRequest();
  }
  return 0;
}

LRESULT NotificationWindow::OnCtlColor(UINT /*msg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& handled) {
  if ( reinterpret_cast<HWND>(lParam) != m_bodyCtrl
    && reinterpret_cast<HWND>(lParam) != m_titleCtrl
    && reinterpret_cast<HWND>(lParam) != m_iconContainer) {
    handled = FALSE;
  }
  if (reinterpret_cast<HWND>(lParam) == m_iconContainer) {
    return reinterpret_cast<LRESULT>(m_iconBgColor.m_hBrush);
  }
  return reinterpret_cast<LRESULT>(m_bodyBgColor.m_hBrush);
}

void NotificationWindow::reloadIcon() {
  m_iconImg.Destroy();
  // set to empty icon because it can happen when it's updated.
  m_icon.SetBitmap(m_iconImg);
  if (m_notificationBody.iconFilePath.empty()) {
    return;
  }
  ATL::CImage origImage;
  if (origImage.Load(m_notificationBody.iconFilePath.c_str()) != S_OK) {
    return;
  }
  CDCHandle screenDC = GetDC();
  CDC myDC;
  myDC.CreateCompatibleDC(screenDC);
  WTL::CBitmap myBitmap;
  myBitmap.CreateCompatibleBitmap(screenDC, DPIAware(kDesiredIconSize), DPIAware(kDesiredIconSize));
  WTL::CBitmapHandle prevBitMap = myDC.SelectBitmap(myBitmap);
  myDC.SetStretchBltMode(HALFTONE);
  origImage.StretchBlt(myDC, WTL::CRect(POINT{ 0, 0 }, DPIAware(SIZE{ kDesiredIconSize, kDesiredIconSize })), SRCCOPY);
  m_iconImg.Attach(myBitmap.Detach());
  myDC.SelectBitmap(prevBitMap);
  m_icon.SetBitmap(m_iconImg);
}

void NotificationWindow::setNotificationBody(const NotificationBody& notificationBody) {
  m_notificationBody = notificationBody;
  m_titleCtrl.SetWindowText(m_notificationBody.title.c_str());
  m_bodyCtrl.SetWindowText(m_notificationBody.body.c_str());
  reloadIcon();
}

NotificationBorderWindow::NotificationBorderWindow(const NotificationBody& notificationBody)
  : m_content(notificationBody)
{
  m_content.onCloseRequest = [this](bool clicked) {
    m_clicked = clicked;
    PostMessage(WM_CLOSE);
  };
  m_content.onSettingsRequest = [this]() {
    if (!!m_onSettingsRequest) {
      m_onSettingsRequest();
    }
  };
}

NotificationBorderWindow:: ~NotificationBorderWindow() {
}

LRESULT NotificationBorderWindow::OnCreate(const CREATESTRUCT* createStruct)
{
  auto windowCoords = getWindowCoordinates();
  MoveWindow(windowCoords.x, windowCoords.y, DPIAware(kWindowWidth), DPIAware(kWindowHeight));

  RECT clientRect;
  GetClientRect(&clientRect);
  // make one pixel border
  clientRect.top += 1;
  clientRect.left += 1;
  clientRect.bottom -= 1;
  clientRect.right -= 1;
  m_content.Create(m_hWnd, clientRect, nullptr, WS_CHILD | WS_VISIBLE);
  auto err = GetLastError();
  SetMsgHandled(false);
  return 0;
}

void NotificationBorderWindow::setSlot(uint8_t value) {
  auto windowCoords = getWindowCoordinates();
  MoveWindow(windowCoords.x, windowCoords.y - value * (DPIAware(kWindowHeight) + /*space between windows*/DPIAware(20)),
    DPIAware(kWindowWidth), DPIAware(kWindowHeight));
}

void NotificationBorderWindow::OnFinalMessage(HWND) {
  if (!!m_onDestroyedCallback) {
    m_onDestroyedCallback(m_clicked);
  }
}

POINT NotificationBorderWindow::getWindowCoordinates() {
  HMONITOR primaryMonitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTOPRIMARY);
  {
    WTL::CDC hdc = GetDC();
    m_dpi = hdc.GetDeviceCaps(LOGPIXELSX);
  }
  MONITORINFO monitorInfo{};
  monitorInfo.cbSize = sizeof(monitorInfo);
  GetMonitorInfo(primaryMonitor, &monitorInfo);
  int windowX = monitorInfo.rcWork.right - DPIAware(kWindowWidth + kWindowPaddingRight);
  int windowY = monitorInfo.rcWork.bottom - DPIAware(kWindowHeight + kWindowPaddingBottom);
  return POINT{ windowX, windowY };
}

namespace {
  class NativeNotificationWindowFactory : public INotificationWindowFactory {
    NotificationWindowPtr create(const NotificationBody& notificationBody,
      const std::function<void(bool clicked)>& onWindowDestroyed,
      const std::function<void()>& onShowSettingsRequested) override {
      auto notificationWindow = std::make_unique<NotificationBorderWindow>(notificationBody);
      notificationWindow->setOnDestroyed(onWindowDestroyed);
      notificationWindow->setOnSettingsRequested(onShowSettingsRequested);
      notificationWindow->Create(nullptr);
      if (nullptr == notificationWindow->operator HWND()) {
        return NotificationWindowPtr();
      }
      notificationWindow->ShowWindow(SW_SHOWNOACTIVATE);
      notificationWindow->UpdateWindow();
      return NotificationWindowPtr(notificationWindow.release());
    }
  };
}

NotificationWindowFactoryPtr ukot::ie_web_notifications::createNativeWindowFactory() {
  return std::make_unique<NativeNotificationWindowFactory>();
}
