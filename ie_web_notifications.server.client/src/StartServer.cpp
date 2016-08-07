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
#include <sergz.utils/utils.h>
#include <sergz.utils/ScopedProcessInformation.h>
#include <sddl.h>
#include "StartServer.h"

namespace {
  const wchar_t* g_kProcessName = L"ie_web_notifications_server.exe";
  struct ScopedSIDBuffer : ukot::utils::Noncopyable {
    ScopedSIDBuffer() : m_sid{ nullptr }{}
    ~ScopedSIDBuffer() {
      if (nullptr == m_sid) {
        LocalFree(m_sid);
      }
    }
    PSID m_sid;
  };
}

// Let's start the server at Low integrity level, so we allow Windows to secure us.
// If there is some volnurability in the server we are protected by Windows.
// Otherwise the user is asked each time about the permission to launch the process.
// There is also a way to configure it in the registry, but I find it inconvenient
// because we need to update the registry each time we change the server location, which
// is annoying during the development.
// http://msdn.microsoft.com/en-us/library/bb250462(v=vs.85).aspx#dse_stlip
#if 0
bool ukot::ie_web_notifications::startProcess() {
  auto currentDllPath = ukot::utils::getDllDirPath();
  auto applicationNamePath = currentDllPath + g_kProcessName;
  auto commandLine = applicationNamePath;
  SECURITY_ATTRIBUTES* saProcess = nullptr;
  SECURITY_ATTRIBUTES* saThread = nullptr;
  BOOL inheritHandles = FALSE;
  DWORD creationFlags = 0;
  void* environment = 0; // inherit env
  const wchar_t* currentDirectory = currentDllPath.c_str();
  STARTUPINFO startupInfo{};
  startupInfo.cb = sizeof(startupInfo);
  ukot::utils::ScopedProcessInformation processInformation{};
  ukot::utils::AccessToken currentProcessAccessToken;
  if (0 == OpenProcessToken(GetCurrentProcess(), MAXIMUM_ALLOWED, &currentProcessAccessToken)) {
    DWORD lastError = GetLastError();
    std::string;
    return false;
  }
  ukot::utils::AccessToken lowIL_token;
  if (0 == DuplicateTokenEx(currentProcessAccessToken.handle(),
    /* desired access */MAXIMUM_ALLOWED, // To request all access rights that are valid for the caller, specify MAXIMUM_ALLOWED.
    /* sa attrs for the new token*/ nullptr, // nothing special so far and don't inherit it
    SecurityImpersonation, TokenPrimary, &lowIL_token)) {
    DWORD lastError = GetLastError();
    std::string;
    return false;
  }
  const wchar_t* lowIL_SID =  L"S-1-16-4096";
  ScopedSIDBuffer pIntegritySid;
  if (0 == ConvertStringSidToSidW(lowIL_SID, &pIntegritySid.m_sid)) {
    DWORD lastError = GetLastError();
    std::string;
    return false;
  }
  TOKEN_MANDATORY_LABEL tokenMandatoryLabel{};
  tokenMandatoryLabel.Label.Attributes = SE_GROUP_INTEGRITY;
  tokenMandatoryLabel.Label.Sid = pIntegritySid.m_sid;
  if (0 == SetTokenInformation(lowIL_token.handle(), TokenIntegrityLevel, &tokenMandatoryLabel,
    sizeof(tokenMandatoryLabel) + GetLengthSid(pIntegritySid.m_sid))) {
    DWORD lastError = GetLastError();
    std::string;
    return false;
  }
  BOOL createProcessResult = CreateProcessAsUserW(lowIL_token.handle(), applicationNamePath.c_str(),
    &commandLine[0], saProcess, saThread, inheritHandles, creationFlags, environment,
    currentDirectory, &startupInfo, &processInformation);
  if (0 == createProcessResult) {
    DWORD lastError = GetLastError();
    std::string;
    return false;
  }
  // succeded

  return true;
}
#endif

bool ukot::ie_web_notifications::startProcess() {
  auto currentDllPath = ukot::utils::getDllDirPath();
  auto applicationNamePath = currentDllPath + g_kProcessName;
  auto commandLine = applicationNamePath;
  SECURITY_ATTRIBUTES* saProcess = nullptr;
  SECURITY_ATTRIBUTES* saThread = nullptr;
  BOOL inheritHandles = FALSE;
  DWORD creationFlags = 0;
  void* environment = 0; // inherit env
  const wchar_t* currentDirectory = currentDllPath.c_str();
  STARTUPINFO startupInfo{};
  startupInfo.cb = sizeof(startupInfo);
  ukot::utils::ScopedProcessInformation processInformation{};
  BOOL createProcessResult = CreateProcessW(applicationNamePath.c_str(), &commandLine[0],
    saProcess, saThread, inheritHandles, creationFlags, environment, currentDirectory, &startupInfo, &processInformation);
  if (0 == createProcessResult) {
    DWORD lastError = GetLastError();
    std::string;
    return false;
  }
  // succeded

  return true;
}
