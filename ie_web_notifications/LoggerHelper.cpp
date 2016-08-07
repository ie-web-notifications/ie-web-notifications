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
#include "LoggerHelper.h"
#include <ie_web_notifications.utils/convertTo.h>

using namespace ukot::ie_web_notifications;

namespace {
  class OutputDebugStringLoggerImpl : public sergz::ILogger {
  public:
    explicit OutputDebugStringLoggerImpl(const std::function<std::wstring()>& getUserData) : m_getUserData(getUserData){}
    void log(LogLevel, const wchar_t* msg, const char* file, int line, const char* funcName) override {
      std::wstringstream ss;
      if (!!m_getUserData) {
        ss << m_getUserData() << L"::";
      }
      ss << util::convertToWString(funcName) << L" >> " << msg;
      OutputDebugStringW(ss.str().c_str());
    }
  private:
    std::function<std::wstring()> m_getUserData;
  };
}

std::unique_ptr<sergz::ILogger> ukot::ie_web_notifications::createOutputDebugStringLoggerImpl(const std::function<std::wstring()>& getUserData) {
  /*
  return std::make_unique<OutputDebugStringLoggerImpl>(getUserData);
  /*/
  return nullptr;
  //*/
}
