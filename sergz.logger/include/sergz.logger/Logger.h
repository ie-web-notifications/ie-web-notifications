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
#include <vector>
#include <memory>

namespace sergz {
  struct ILogger {
    virtual ~ILogger(){}

    enum class LogLevel { Trace, Debug, Info, Warn, Error, Fatal };
    virtual void log(LogLevel, const wchar_t* msg, const char* file = nullptr, int line = 0, const char* funcName = nullptr) = 0;

    void fatal(const wchar_t* msg, const char* file = nullptr, int line = 0, const char* funcName = nullptr) {
      log(LogLevel::Fatal, msg, file, line, funcName);
    }
    void error(const wchar_t* msg, const char* file = nullptr, int line = 0, const char* funcName = nullptr) {
      log(LogLevel::Error, msg, file, line, funcName);
    }
    void warn(const wchar_t* msg, const char* file = nullptr, int line = 0, const char* funcName = nullptr) {
      log(LogLevel::Warn, msg, file, line, funcName);
    }
    void info(const wchar_t* msg, const char* file = nullptr, int line = 0, const char* funcName = nullptr) {
      log(LogLevel::Info, msg, file, line, funcName);
    }
    void debug(const wchar_t* msg, const char* file = nullptr, int line = 0, const char* funcName = nullptr) {
      log(LogLevel::Debug, msg, file, line, funcName);
    }
    void trace(const wchar_t* msg, const char* file = nullptr, int line = 0, const char* funcName = nullptr) {
      log(LogLevel::Trace, msg, file, line, funcName);
    }
  };

  struct ILoggerFactory {
    virtual ~ILoggerFactory(){}
    typedef std::pair<std::wstring, std::wstring> Property;
    typedef std::vector<Property> Properties;
    virtual void propertiesFile(const std::wstring& propertiesFile, const Properties& userProperties) = 0;
    virtual std::unique_ptr<ILogger> getLogger(const wchar_t* name) = 0;
  };
}

