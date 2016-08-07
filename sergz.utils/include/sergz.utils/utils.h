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
#include <guiddef.h>
#include <urlmon.h>
#include <string>

namespace ukot { namespace utils {
  /// Get named GUID for debug log.
  /// Attempt to translate it through standard Registry lookup, or our
  /// custom Registry repository of interface, GUID, Co-class names.
  const wchar_t* getIIDName(REFIID riid);
  std::wstring toStdWString(PARSEACTION value);
  std::wstring combineUrlFlagsToStdWString(DWORD value);
  std::wstring queryOptionToStdWString(QUERYOPTION value);
  std::wstring getDllDirPath();
  std::wstring getExeDirPath();
  std::wstring getAppDataLocalLowPath();
  std::wstring getKnownFolder(REFIID riid);

  // moves `begin` argument to the the first non-space character position.
  void leftTrim(std::wstring::const_iterator& begin, const std::wstring::const_iterator end);

  // Case insensitive version of beginsWith. It moves `begin` argument to the first non-equal.
  bool beginsWith_ci(std::wstring::const_iterator& begin, const std::wstring::const_iterator end, const std::wstring& match);
}}