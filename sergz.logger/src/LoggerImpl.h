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
#include "../include/sergz.logger/Logger.h"
#include <string>
// 1> $(SolutionDir)vendors\log4cplus\repo\include\log4cplus\helpers\timehelper.h(117) 
//    : warning C4244 : 'return' : conversion from '__int64' to 'long', possible loss of data
#pragma warning(push)
#pragma warning(disable: 4244)
#include <log4cplus\logger.h>
#pragma warning(pop)

namespace ukot { namespace logger {
    class LoggerFactoryImpl;
    class LoggerImpl final : public ILogger {
    public:
      explicit LoggerImpl(const std::wstring& name);
      virtual ~LoggerImpl();
      virtual void log(LogLevel, const wchar_t* msg, const char* file = nullptr, int line = 0, const char* funcName = nullptr) override;
    private:
      std::wstring m_name;
      std::unique_ptr<::log4cplus::Logger> m_logger;
    };
  }
}