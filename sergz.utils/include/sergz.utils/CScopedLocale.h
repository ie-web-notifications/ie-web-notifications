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
#include <clocale>

namespace ukot { namespace util {
  class CScopedLocale {
  public:
    CScopedLocale(int category = LC_ALL, const wchar_t* locale = L"C")
      : m_value(_wcreate_locale(category, locale))
    {}
    ~CScopedLocale() {
      _free_locale(m_value);
    }
    _locale_t handle() {
      return m_value;
    }
    _locale_t m_value;
  };
}}
