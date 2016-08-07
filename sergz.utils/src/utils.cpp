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
#include "../include/sergz.utils/utils.h"
#include <mutex>
#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <limits>
#include <objbase.h>
#include <atlbase.h>
#include <Wininet.h>
#include <Pathcch.h>
#include <Shlobj.h>
#include <cwctype>

const wchar_t* ukot::utils::getIIDName(REFIID riid) {
  static std::mutex s_mutex;
  std::lock_guard<std::mutex> lock(s_mutex);
  static ATL::CSimpleMap<GUID, ATL::CComBSTR> s_cache;
  {
    auto value = s_cache.Lookup(riid);
    if (!!value) {
      return value;
    }
  }
  wchar_t szName[80] = { 0 };
  wchar_t wszGUID[50] = { 0 };
  ::StringFromGUID2(riid, wszGUID, 50);
  // Attempt to find it in the interfaces section
  ATL::CRegKey key;
  DWORD dwType = 0;
  DWORD dwCount = sizeof(szName) - sizeof(wchar_t);
  key.Open(HKEY_CLASSES_ROOT, _T("Interface"), KEY_READ);
  if (szName[0] == '\0' && key.Open(key, wszGUID, KEY_READ) == NOERROR) {
    ::RegQueryValueEx(key.m_hKey, NULL, NULL, &dwType, (LPBYTE)szName, &dwCount);
  }
  // Attempt to find it in the CLSID section
  key.Open(HKEY_CLASSES_ROOT, _T("CLSID"), KEY_READ);
  if (szName[0] == '\0' && key.Open(key, wszGUID, KEY_READ) == NOERROR) {
    ::RegQueryValueEx(key.m_hKey, NULL, NULL, &dwType, (LPBYTE)szName, &dwCount);
  }
  // Attempt to find it in our Named GUIDs section
  key.Open(HKEY_CLASSES_ROOT, _T("Named GUIDs"), KEY_READ);
  if (szName[0] == '\0' && key.Open(key, wszGUID, KEY_READ) == NOERROR) {
    ::RegQueryValueEx(key.m_hKey, NULL, NULL, &dwType, (LPBYTE)szName, &dwCount);
  }
  if (szName[0] == '\0') {
    wcscpy_s(szName, 80, ATL::CW2T(wszGUID));
  }
  s_cache.Add(riid, szName);
  return s_cache.Lookup(riid);
}

std::wstring ukot::utils::toStdWString(PARSEACTION value) {
  switch (value) {
    case PARSE_CANONICALIZE      : return L"PARSE_CANONICALIZE   ";
    case PARSE_FRIENDLY          : return L"PARSE_FRIENDLY       ";
    case PARSE_SECURITY_URL      : return L"PARSE_SECURITY_URL   ";
    case PARSE_ROOTDOCUMENT      : return L"PARSE_ROOTDOCUMENT   ";
    case PARSE_DOCUMENT          : return L"PARSE_DOCUMENT       ";
    case PARSE_ANCHOR            : return L"PARSE_ANCHOR         ";
    case PARSE_ENCODE_IS_UNESCAPE: return L"PARSE_ENCODE         ";
    case PARSE_DECODE_IS_ESCAPE  : return L"PARSE_DECODE         ";
    case PARSE_PATH_FROM_URL     : return L"PARSE_PATH_FROM_URL  ";
    case PARSE_URL_FROM_PATH     : return L"PARSE_URL_FROM_PATH  ";
    case PARSE_MIME              : return L"PARSE_MIME           ";
    case PARSE_SERVER            : return L"PARSE_SERVER         ";
    case PARSE_SCHEMA            : return L"PARSE_SCHEMA         ";
    case PARSE_SITE              : return L"PARSE_SITE           ";
    case PARSE_DOMAIN            : return L"PARSE_DOMAIN         ";
    case PARSE_LOCATION          : return L"PARSE_LOCATION       ";
    case PARSE_SECURITY_DOMAIN   : return L"PARSE_SECURITY_DOMAIN";
    case PARSE_ESCAPE            : return L"PARSE_ESCAPE         ";
    case PARSE_UNESCAPE          : return L"PARSE_UNESCAPE       ";
    default:
      ;
  }
  return L"Unknown parse action";
}

#define APPEND_COMBINE_URL_FLAGS(x) if ((value & x) == x) { \
ss << L ## #x L" |"; \
}

std::wstring ukot::utils::combineUrlFlagsToStdWString(DWORD value) {
  std::wstringstream ss;
  APPEND_COMBINE_URL_FLAGS(ICU_BROWSER_MODE);
  APPEND_COMBINE_URL_FLAGS(ICU_DECODE);
  APPEND_COMBINE_URL_FLAGS(ICU_ENCODE_PERCENT);
  APPEND_COMBINE_URL_FLAGS(ICU_ENCODE_SPACES_ONLY);
  APPEND_COMBINE_URL_FLAGS(ICU_NO_ENCODE);
  APPEND_COMBINE_URL_FLAGS(ICU_NO_META);
  auto retValue = ss.str();
  if (!retValue.empty()) {
    retValue.resize(retValue.size() - 2);
  }
  return retValue;
}

std::wstring ukot::utils::queryOptionToStdWString(QUERYOPTION value) {
  switch (value) {
    case QUERY_EXPIRATION_DATE             : return L"QUERY_EXPIRATION_DATE             ";
    case QUERY_TIME_OF_LAST_CHANGE         : return L"QUERY_TIME_OF_LAST_CHANGE         ";
    case QUERY_CONTENT_ENCODING            : return L"QUERY_CONTENT_ENCODING            ";
    case QUERY_CONTENT_TYPE                : return L"QUERY_CONTENT_TYPE                ";
    case QUERY_REFRESH                     : return L"QUERY_REFRESH                     ";
    case QUERY_RECOMBINE                   : return L"QUERY_RECOMBINE                   ";
    case QUERY_CAN_NAVIGATE                : return L"QUERY_CAN_NAVIGATE                ";
    case QUERY_USES_NETWORK                : return L"QUERY_USES_NETWORK                ";
    case QUERY_IS_CACHED                   : return L"QUERY_IS_CACHED                   ";
    case QUERY_IS_INSTALLEDENTRY           : return L"QUERY_IS_INSTALLEDENTRY           ";
    case QUERY_IS_CACHED_OR_MAPPED         : return L"QUERY_IS_CACHED_OR_MAPPED         ";
    case QUERY_USES_CACHE                  : return L"QUERY_USES_CACHE                  ";
    case QUERY_IS_SECURE                   : return L"QUERY_IS_SECURE                   ";
    case QUERY_IS_SAFE                     : return L"QUERY_IS_SAFE                     ";
    case QUERY_USES_HISTORYFOLDER          : return L"QUERY_USES_HISTORYFOLDER          ";
    case QUERY_IS_CACHED_AND_USABLE_OFFLINE: return L"QUERY_IS_CACHED_AND_USABLE_OFFLINE";
  default:
    ;
  }
  return L"Unknown query option";
}

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

namespace {
  std::wstring getModulePath(HINSTANCE hInstance) {
    std::vector<wchar_t> path(MAX_PATH, L'\0');
    DWORD length = GetModuleFileNameW(hInstance, path.data(), static_cast<DWORD>(path.size()));

    while (length == path.size() && path.size() < std::numeric_limits<std::vector<wchar_t>::size_type>::max() / 2) {
      // last error ERROR_INSUFFICIENT_BUFFER is not supported by WinXP
      // Buffer too small, double buffer size and try again
      path.resize(path.size() * 2);
      length = GetModuleFileNameW(hInstance, path.data(), static_cast<DWORD>(path.size()));
    }

    auto it = std::find(path.rbegin(), path.rend(), L'\\');
    if (path.rend() == it) {
      return std::wstring();
    }

    return std::wstring(path.begin(), it.base());
  }
}

std::wstring ukot::utils::getDllDirPath() {
  return getModulePath((HINSTANCE)&__ImageBase);
}

std::wstring ukot::utils::getExeDirPath() {
  return getModulePath(nullptr);
}

std::wstring ukot::utils::getAppDataLocalLowPath() {
  return getKnownFolder(FOLDERID_LocalAppDataLow);
}

std::wstring ukot::utils::getKnownFolder(REFIID folderID) {
  struct CoTaskMem {
    ~CoTaskMem() {
      if (nullptr != p) {
        CoTaskMemFree(p);
      }
    }
    wchar_t* p;
  } strBuffer = {};
  if (S_OK != SHGetKnownFolderPath(folderID, /* flags */ 0, /* token */ 0, &strBuffer.p)) {
    return std::wstring();
  }
  return strBuffer.p;
}

void ukot::utils::leftTrim(std::wstring::const_iterator& begin, const std::wstring::const_iterator end) {
  if (begin != end) {
    // eat spaces
    while (std::iswspace(*begin) && ++begin != end && *begin != 0);
  }
}

bool ukot::utils::beginsWith_ci(std::wstring::const_iterator& ii, const std::wstring::const_iterator ii_end, const std::wstring& match) {
  auto ii_match = match.begin();
  auto ii_matchEnd = match.end();
  for (;
    ii != ii_end && ii_match != ii_matchEnd // don't exceed the length
    && std::towlower(*ii) == *ii_match; // actual character comparison
  ++ii, ++ii_match);
  return ii_match == ii_matchEnd;
};