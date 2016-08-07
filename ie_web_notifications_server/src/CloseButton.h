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
// wtl headers
#include <atlapp.h>
#include <atlframe.h>
#include <atlmisc.h>
#include <atlctrls.h>
#include <atlcrack.h>
#include <functional>

namespace ukot { namespace ie_web_notifications {
  class CloseButton : public ATL::CWindowImpl<CloseButton, WTL::CButton>{
  public:
    DECLARE_WND_SUPERCLASS(NULL, CButton::GetWndClassName())
    std::function<void()> clicked;
    BEGIN_MSG_MAP(CloseButton)
      MESSAGE_HANDLER(WM_LBUTTONDOWN, OnClick)
    END_MSG_MAP()
  private:
    LRESULT OnClick(UINT /*msg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*handled*/);
  };
}}