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

#include "Server.h"
#include "NotificationWindow.h"
#include "ServerClient.h"
#include <sergz.utils/EventWithSetter.h>
#include <ShellScalingAPI.h>
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

namespace {
  const wchar_t* g_kSecondInstanceMutexName = L"Local\\ie_web_notifications.secondInstanceMutex";
  const wchar_t* g_kPipeName = L"\\\\.\\pipe\\ie_web_notification.server.pipe";

  struct ScopedAtlAxInitializer
  {
    ScopedAtlAxInitializer() {
      ATL::AtlAxWinInit();
    }
    ~ScopedAtlAxInitializer() {
      ATL::AtlAxWinTerm();
    }
  };

  class MyAtlModule : public ATL::CAtlExeModuleT<MyAtlModule>
  {
  public:
    MyAtlModule() {
    }
    static HRESULT InitializeCom() throw() {
      // The default implementation initializes multithreaded version but
      // in this case hosted ActiveX does not properly work.
      return CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    }
  private:
    ScopedAtlAxInitializer m_scopedAtlAxInit;
  } _AtlModule;

  class CommandLineArgs {
  public:
    explicit CommandLineArgs(const wchar_t* commandLine) {
      if (!commandLine) {
        return;
      }
      std::wstring strCmdLine = commandLine;
      m_showSettings = std::wstring::npos != strCmdLine.find(L"--settings");
    }
    bool showSettings() const {
      return m_showSettings;
    }
  private:
    bool m_showSettings = false;
  };

}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR commandLine, int) {
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
  CommandLineArgs cmdArgs(GetCommandLineW());

  if (cmdArgs.showSettings()) {
    ukot::ie_web_notifications::ServerClient serverClient;
    ukot::utils::EventWithSetter eventWithSetter;
    {
      auto setter = eventWithSetter.getSetter();
      serverClient.showSettings([setter]{
        setter->set();
      });
    }
    eventWithSetter.wait(std::chrono::seconds(5));
    return 0;
  }

  ukot::utils::MutexHandle secondInstance{ CreateMutexW(/*sec attr*/nullptr,
    /*is intial owner*/FALSE, g_kSecondInstanceMutexName) };
  if (!secondInstance) {
    // we have a problem
    return 1;
  }
  {
    DWORD lastError = GetLastError();
    if (ERROR_ALREADY_EXISTS == lastError) {
      // second instance
      return 0;
    }
  }
  WTL::AtlInitCommonControls(ICC_STANDARD_CLASSES);
  _AtlModule.InitializeCom();
  ukot::ie_web_notifications::Server::Params params;
  params.pipeName = g_kPipeName;
  params.messageLoopWaitMsec = std::chrono::milliseconds(100);
  params.exitTimeout = std::chrono::seconds(2);
  {
    ukot::ie_web_notifications::Settings settings;
    ukot::ie_web_notifications::Server server(params, settings);
    server.runMessageLoop();
  }
  _AtlModule.UninitializeCom();
  return 0;
}