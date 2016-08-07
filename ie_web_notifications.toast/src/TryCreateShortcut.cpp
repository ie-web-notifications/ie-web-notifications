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
#include <Psapi.h>
#include <ShObjIdl.h>
#include <propvarutil.h>
#include <propkey.h>
#include <sergz.utils/utils.h>

namespace ukot { namespace ie_web_notifications {
  HRESULT tryCreateShortcut();
}}

#define EXPECT_HR_SUCCEEDED(expr) if (FAILED(hr = (expr))) { return hr; }

namespace {
  // Install the shortcut
  HRESULT InstallShortcut(const std::wstring& shortcutPath)
  {
    auto currentDllPath = ukot::utils::getDllDirPath();
    auto applicationNamePath = currentDllPath + L"ie_web_notifications_server.exe";

    ATL::CComPtr<IShellLink> shellLink;
    HRESULT hr = S_OK;
    EXPECT_HR_SUCCEEDED(shellLink.CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER));
    EXPECT_HR_SUCCEEDED(shellLink->SetPath(applicationNamePath.c_str()));
    EXPECT_HR_SUCCEEDED(shellLink->SetArguments(L"--settings"));
    EXPECT_HR_SUCCEEDED(shellLink->SetWorkingDirectory(currentDllPath.c_str()));
    EXPECT_HR_SUCCEEDED(shellLink->SetDescription(L"IE Web Notifications"));
    EXPECT_HR_SUCCEEDED(shellLink->SetIconLocation(applicationNamePath.c_str(), 0));

    ATL::CComPtr<IPropertyStore> propertyStore;
    EXPECT_HR_SUCCEEDED(shellLink.QueryInterface(&propertyStore));

    struct PropVariant :PROPVARIANT {
      ~PropVariant() {
        PropVariantClear(this);
      }
      bool initialzed = false;
    } appIdPropVar{};
    const wchar_t kAppId[] = L"ukot.ie_web_notifications";
    EXPECT_HR_SUCCEEDED(InitPropVariantFromString(kAppId, &appIdPropVar));
    appIdPropVar.initialzed = true;
    EXPECT_HR_SUCCEEDED(propertyStore->SetValue(PKEY_AppUserModel_ID, appIdPropVar));
    EXPECT_HR_SUCCEEDED(propertyStore->Commit());
    ATL::CComPtr<IPersistFile> persistFile;
    EXPECT_HR_SUCCEEDED(shellLink.QueryInterface(&persistFile));
    EXPECT_HR_SUCCEEDED(persistFile->Save(shortcutPath.c_str(), TRUE));

    return hr;
  }
}

#include <Knownfolders.h>
HRESULT ukot::ie_web_notifications::tryCreateShortcut() 
{
  auto startMenu_ProgramsPath = utils::getKnownFolder(FOLDERID_CommonPrograms);
  auto shortcutPath = startMenu_ProgramsPath + L"\\IE Web Notifications\\IE Web Notifications.lnk";

  DWORD attributes = GetFileAttributes(shortcutPath.c_str());
  bool fileExists = attributes < 0xFFFFFFF;

  HRESULT hr = S_OK;
  if (!fileExists)
  {
    hr = InstallShortcut(shortcutPath);
  }
  else
  {
    hr = S_FALSE;
  }
  return hr;
}