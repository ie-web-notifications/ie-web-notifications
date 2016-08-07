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
#include <sergz.utils/DPIAwareness.h>
// wtl headers
#include <atlapp.h>
#include <atlframe.h>
#include <atlmisc.h>
#include <atlctrls.h>
#include <atlcrack.h>
#include "CloseButton.h"
#include <MsHtmdid.h>

namespace ukot { namespace ie_web_notifications {
  class Settings;

  enum
  {
    // ID of HTMLDocument ActiveX control, it's used for event binding.
    kHTMLDocumentCtrlID = 101
  };

  class SettingsWindow : public ATL::CWindowImpl<SettingsWindow>
    , ATL::IDispEventImpl<kHTMLDocumentCtrlID, SettingsWindow, &DIID_HTMLDocumentEvents2, &LIBID_MSHTML, 4, 0>
    , protected utils::DpiAwareness
  {
  public:
    explicit SettingsWindow(Settings& settings);
    ~SettingsWindow();
    BEGIN_MSG_MAP(SettingsWindow)
      MSG_WM_CREATE(OnCreate)
      MSG_WM_DESTROY(OnDestroy)
    END_MSG_MAP()

    BEGIN_SINK_MAP(SettingsWindow)
      SINK_ENTRY_EX(kHTMLDocumentCtrlID, DIID_HTMLDocumentEvents2, DISPID_HTMLDOCUMENTEVENTS2_ONCLICK, OnHTMLDocumentClick)
      SINK_ENTRY_EX(kHTMLDocumentCtrlID, DIID_HTMLDocumentEvents2, DISPID_HTMLDOCUMENTEVENTS2_ONSELECTSTART, OnHTMLDocumentSelectStart)
    END_SINK_MAP()

    void SetOnClose(const std::function<void()>& callback) {
      m_onCloseCallback = callback;
    }
  private:
    LRESULT OnCreate(const CREATESTRUCT* createStruct);
    LRESULT OnClick(UINT msg, WPARAM wParam, LPARAM lParam, BOOL& handled);
    void OnDestroy();

    void __stdcall OnHTMLDocumentClick(IHTMLEventObj* pEvtObj);
    void __stdcall OnHTMLDocumentSelectStart(IHTMLEventObj* pEvtObj);
  private:
    std::wstring m_htmlPage;
    CBrush m_bgColor;
    ATL::CAxWindow m_axIE;
    std::function<void()> m_onCloseCallback;
    Settings& m_settings;
  };

  typedef ATL::CWinTraits<WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_TOOLWINDOW | WS_EX_TOPMOST> SettingsBorderWindowStyles;
  class SettingsBorderWindow
    : public ATL::CWindowImpl<SettingsBorderWindow, ATL::CWindow, SettingsBorderWindowStyles>
    , protected utils::DpiAwareness
  {
  public:
    DECLARE_WND_CLASS_EX(/*generate class name*/nullptr, CS_DROPSHADOW, WHITE_BRUSH);
    explicit SettingsBorderWindow(Settings& settings);
    ~SettingsBorderWindow();

    BEGIN_MSG_MAP(SettingsBorderWindow)
      MSG_WM_CREATE(OnCreate)
    END_MSG_MAP()
    void SetOnDestroyed(const std::function<void()>& callback) {
      m_onDestroyedCallback = callback;
    }

  private:
    LRESULT OnCreate(const CREATESTRUCT* createStruct);
    void OnFinalMessage(HWND) override;

    // returns {windowX, windowY} on the monitor
    POINT getWindowCoordinates();
  private:
    // m_content is used as a holder of all children and we need it to have a border.
    // It seems the most correct way to have a border to set WS_POPUPWINDOW style
    // and paint the border in WM_NCPAINT but it simply does not work here.
    SettingsWindow m_content;
    std::function<void()> m_onDestroyedCallback;
  };
}}