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
#include <ie_web_notifications.utils/convertTo.h>

std::wstring ukot::ie_web_notifications::util::convertToWString(const BSTR value) {
  if (!value) {
    assert(!value && "BSTR string must be not nullptr");
    return std::wstring();
  }
  return std::wstring(value, SysStringLen(value));
}

std::wstring ukot::ie_web_notifications::util::convertToWString(const std::string& value) {
  int numWideChars = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.length()) /*bytes*/, nullptr, 0 /*characters*/);
  std::wstring res(numWideChars, L'\0'); // fill by zeros
  MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.length()), &res[0], numWideChars /*characters*/);
  return res;
}

std::string ukot::ie_web_notifications::util::convertToString(const std::wstring& value) {
  int numBytes = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.length()) /*characters*/, nullptr, 0 /*bytes*/, NULL, NULL);
  std::string res(numBytes, '\0'); // fill by zeros
  WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.length()) /*characters*/, &res[0], numBytes /*bytes*/, NULL, NULL);
  return res;
}

ATL::CComBSTR ukot::ie_web_notifications::util::convertToBSTR(const std::wstring& value) {
  return ATL::CComBSTR{ static_cast<int>(value.length()), value.c_str() };
}
