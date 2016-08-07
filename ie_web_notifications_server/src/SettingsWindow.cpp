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

#include "SettingsWindow.h"
#include "ScriptingSettings.h"
#include <sergz.utils/utils.h>
#include <fstream>
#include "PermissionDB.h"

using namespace ukot::ie_web_notifications;

namespace {
  uint32_t kWindowWidth = 400;
  uint32_t kWindowHeight = 600;
  uint32_t kWindowPaddingRight = 280 + 50 + 50;
  uint32_t kWindowPaddingBottom = 40;
}

SettingsWindow::SettingsWindow(Settings& settings)
  : m_settings(settings)
{
  auto appDir = ukot::utils::getExeDirPath();
  auto htmlFile = appDir + L'\\' + L"SettingsWindow.html";
  std::wifstream ifs(htmlFile);
  assert(ifs.good() && "Cannot open SettingsWindow.html file");
  if (ifs.good()) {
    m_htmlPage.assign((std::istreambuf_iterator<wchar_t>(ifs)), std::istreambuf_iterator<wchar_t>());
  } else {
    m_htmlPage = L"<html><body style='overflow:none;'>Cannot open html file for settings window!</body></html>";
  }
}

SettingsWindow::~SettingsWindow() {

}

LRESULT SettingsWindow::OnCreate(const CREATESTRUCT* /*createStruct*/) {
  {
    WTL::CDC hdc = GetDC();
    m_dpi = hdc.GetDeviceCaps(LOGPIXELSX);
  }
  m_bgColor.CreateSolidBrush(RGB(255, 255, 255));

  m_axIE.Create(m_hWnd, DPIAware(CRect(CPoint(0, 0), CSize(kWindowWidth, kWindowHeight))),
    L"", WS_CHILD | WS_VISIBLE, 0, kHTMLDocumentCtrlID);
  m_axIE.CreateControl((L"mshtml:" + m_htmlPage).c_str());

  ATL::CComPtr<IDispatch> scriptingSettings;
  HRESULT hr = ukot::atl::SharedObject<ScriptingSettingsImpl>::Create(&scriptingSettings, [this]{
    if (!!m_onCloseCallback) {
      m_onCloseCallback();
    }
  }, m_settings);
  m_axIE.SetExternalDispatch(scriptingSettings);

  ATL::CComPtr<IAxWinAmbientDispatch> axWinAmbient;
  if (SUCCEEDED(m_axIE.QueryHost(&axWinAmbient))) {
    // disable web browser context menu
    axWinAmbient->put_AllowContextMenu(VARIANT_FALSE);
    // make web browser DPI aware, so the browser itself sets zoom level and
    // cares about rendering (not zooming) in the proper size.
    DWORD docFlags;
    axWinAmbient->get_DocHostFlags(&docFlags);
    docFlags |= DOCHOSTUIFLAG_DPI_AWARE;
    // remove DOCHOSTUIFLAG_SCROLL_NO, so it's scrollable
    docFlags &= ~DOCHOSTUIFLAG_SCROLL_NO;
    axWinAmbient->put_DocHostFlags(docFlags);
  }
  // kHTMLDocumentCtrlID works here
  AtlAdviseSinkMap(this, true);

  SetMsgHandled(false);
  return 0;
}

void __stdcall SettingsWindow::OnHTMLDocumentClick(IHTMLEventObj* eventObject) {
  // stop propagating the event since it's handled by us and should not cause any other actions.
  if (!eventObject)
    return;
  //eventObject->put_cancelBubble(VARIANT_TRUE);
  //eventObject->put_returnValue(ATL::CComVariant(false));
}

void __stdcall SettingsWindow::OnHTMLDocumentSelectStart(IHTMLEventObj* eventObject) {
  if (!eventObject)
    return;
  // disable selecting
  eventObject->put_cancelBubble(VARIANT_TRUE);
  eventObject->put_returnValue(ATL::CComVariant(false));
}

void SettingsWindow::OnDestroy() {
  AtlAdviseSinkMap(this, false);
  // and proceed as usual
  SetMsgHandled(false);
}

SettingsBorderWindow::SettingsBorderWindow(Settings& settings)
  : m_content(settings)
{
  m_content.SetOnClose([this]{
    PostMessage(WM_CLOSE);
  });
}

SettingsBorderWindow::~SettingsBorderWindow() {

}

LRESULT SettingsBorderWindow::OnCreate(const CREATESTRUCT* createStruct) {
  {
    WTL::CDC hdc = GetDC();
    m_dpi = hdc.GetDeviceCaps(LOGPIXELSX);
  }
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

void SettingsBorderWindow::OnFinalMessage(HWND) {
  if (!!m_onDestroyedCallback) {
    m_onDestroyedCallback();
  }
}

POINT SettingsBorderWindow::getWindowCoordinates() {
  HMONITOR primaryMonitor = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTOPRIMARY);
  MONITORINFO monitorInfo{};
  monitorInfo.cbSize = sizeof(monitorInfo);
  GetMonitorInfo(primaryMonitor, &monitorInfo);
  int windowX = monitorInfo.rcWork.right - DPIAware(kWindowWidth + kWindowPaddingRight);
  int windowY = monitorInfo.rcWork.bottom - DPIAware(kWindowHeight + kWindowPaddingBottom);
  return POINT{ windowX, windowY };
}