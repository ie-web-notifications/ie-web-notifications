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

#include "Settings.h"
#include <fstream>
#include "Utils.h"

using namespace ukot::ie_web_notifications;

namespace {
  std::wstring getSettingsFilePath() {
    return getDataFolderPath() + L"\\settings";
  }

  void processSetting(const std::string& line, const std::function<void(const std::string& name, const std::string& value)>& fn) {
    if (line.empty() || !fn) {
      return;
    }
    auto begin = line.begin();
    auto end = line.end();
    auto assignSignIt = std::find(begin, end , '=');
    auto valueBeginsAt = assignSignIt != end ? assignSignIt + 1 : assignSignIt;
    fn(std::string(begin, assignSignIt), std::string(valueBeginsAt, end));
  }
  bool settingValueAsBool(const std::string& value) {
    return !value.empty() && value[0] == '1';
  }

  const std::string kToastSettingName = "use_toast_notifications";
}

Settings::Settings()
  : m_filePath(getSettingsFilePath())
{
  load();
  if (!(m_nativeNotificationWindowFactory = createD2DWindowFactory())) {
    m_nativeNotificationWindowFactory = createNativeWindowFactory();
  }
  m_toastNotificationWindowFactory = createToastWindowFactory();
}

Settings::~Settings(){

}

void Settings::setToastNotificationsEnabled(bool enabled) {
  // do nothing if the value is the same
  if (m_isToastNotificationsEnabled == enabled) {
    return;
  }

  bool oldValue = m_isToastNotificationsEnabled;
  m_isToastNotificationsEnabled = enabled && canSupportToastNotificationsWindowFactory();

  if (oldValue != m_isToastNotificationsEnabled && !!toastNotificationsEnabledChanged) {
    toastNotificationsEnabledChanged();
  }
  save();
}

bool Settings::canSupportToastNotificationsWindowFactory() {
  return !!m_toastNotificationWindowFactory;
}

INotificationWindowFactory& Settings::notificationWindowFactory() {
  return (m_isToastNotificationsEnabled && !!m_toastNotificationWindowFactory) ?
    *m_toastNotificationWindowFactory : *m_nativeNotificationWindowFactory;
}

void Settings::load() {
  std::ifstream ifs(m_filePath, std::ifstream::in);
  while (ifs.good()) {
    std::string line;
    std::getline(ifs, line);
    processSetting(line, [this](const std::string& name, const std::string& value){
      if (name == kToastSettingName) {
        m_isToastNotificationsEnabled = settingValueAsBool(value);
      }
    });
  }
  ifs.close();
}

void Settings::save() {
  std::ofstream ofs(m_filePath, std::ofstream::out);
  if (!ofs.is_open()) {
    return;
  }

  {
    std::string line;
    line += kToastSettingName;
    line += "=";
    line.push_back(m_isToastNotificationsEnabled ? '1' : '0');
    line.push_back('\n');
    ofs << line;
  }
  ofs.flush();
  ofs.close();

}
