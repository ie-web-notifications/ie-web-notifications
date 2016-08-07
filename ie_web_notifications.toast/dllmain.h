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

class Cie_web_notificationstoastModule : public ATL::CAtlDllModuleT< Cie_web_notificationstoastModule >
{
public :
  DECLARE_LIBID(__uuidof(ukot::ie_web_notifications::com::ie_web_notificationstoastLib))
  DECLARE_REGISTRY_APPID_RESOURCEID(IDR_IE_WEB_NOTIFICATIONSTOAST, "{FD97FBB6-027F-429F-A7D4-512D99ED8450}")
};

extern class Cie_web_notificationstoastModule _AtlModule;
