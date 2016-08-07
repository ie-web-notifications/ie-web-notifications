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
#include <Permission.h>
#include <map>
#include <string>
#include <functional>
#include <sergz.utils/Noncopyable.h>

namespace ukot { namespace ie_web_notifications {
  class PermissionDB : utils::Noncopyable {
  public:
    typedef std::map<std::string, Permission> DB;
    PermissionDB();
    void setPermission(const std::string& origin, Permission value);
    Permission getPermission(const std::string& origin) const;
    size_t count() const {
      return m_db.size();
    }
    const DB::value_type& operator[](size_t i) const {
      // hack
      auto it = m_db.begin();
      size_t c = 0;
      while (c++ < i) ++it;
      return *it;
    }
    void remove(const std::string& origin);
    std::function<void()> onChanged;
  private:
    DB m_db;
    std::wstring m_dbFilePath;
  };
}}