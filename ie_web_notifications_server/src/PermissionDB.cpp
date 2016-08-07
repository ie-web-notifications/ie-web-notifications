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

#include "PermissionDB.h"
#include <fstream>
#include "Utils.h"
using namespace ukot::ie_web_notifications;

namespace {
  std::wstring getDbFilePath() {
    return getDataFolderPath() + L"\\db";
  }

  void writeDown(const std::wstring& fileName, const PermissionDB::DB& db) {
    std::ofstream ofs(fileName, std::ofstream::out);
    if (!ofs.is_open()) {
      return;
    }
    for (const auto& value : db) {
      std::string line;
      line.push_back(static_cast<char>(value.second) + '0');
      line += value.first;
      line.push_back('\n');
      ofs << line;
    }
    ofs.flush();
    ofs.close();
  }
}

PermissionDB::PermissionDB()
  : m_dbFilePath(getDbFilePath())
{
  std::ifstream ifs(m_dbFilePath, std::ifstream::in);
  while (ifs.good()) {
    std::string line;
    std::getline(ifs, line);
    if (!line.empty()) {
      m_db.emplace(make_pair(line.substr(1), static_cast<Permission>(line[0] - '0')));
    }
  }
  ifs.close();
}

Permission PermissionDB::getPermission(const std::string& origin) const {
  auto ii = m_db.find(origin);
  if (m_db.end() == ii) {
    return Permission::default;
  }
  return ii->second;
}

void PermissionDB::setPermission(const std::string& origin, Permission value) {
  auto ii = m_db.find(origin);
  if (m_db.end() == ii) {
    m_db.emplace(make_pair(origin, value));
  } else {
    ii->second = value;
  }
  writeDown(m_dbFilePath, m_db);
  if (!!onChanged) {
    onChanged();
  }
}

void PermissionDB::remove(const std::string& origin) {
  auto ii = m_db.find(origin);
  if (m_db.end() == ii) {
    return;
  }
  m_db.erase(ii);
  writeDown(m_dbFilePath, m_db);
  if (!!onChanged) {
    onChanged();
  }
}
