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
#include "Logger.h"
#include <memory>
#include <sstream>

namespace sergz {
  class LogStream {
  public:
    LogStream(
      ILogger* logger,
      const ILogger::LogLevel level,
      const char* file = nullptr,
      int line = 0,
      const char* funcName = nullptr)
      : m_stream(logger, level, file, line, funcName)
    {
    }

    LogStream &operator<<(bool                t) { m_stream.ts << (t ? L"true" : L"false"); return *this; }
    LogStream &operator<<(wchar_t             t) { m_stream.ts << t; return *this; }
    LogStream &operator<<(int16_t             t) { m_stream.ts << t; return *this; }
    LogStream &operator<<(uint16_t            t) { m_stream.ts << t; return *this; }
    LogStream &operator<<(int32_t             t) { m_stream.ts << t; return *this; }
    LogStream &operator<<(uint32_t            t) { m_stream.ts << t; return *this; }
    LogStream &operator<<(int64_t             t) { m_stream.ts << t; return *this; }
    LogStream &operator<<(uint64_t            t) { m_stream.ts << t; return *this; }
    LogStream &operator<<(float               t) { m_stream.ts << t; return *this; }
    LogStream &operator<<(double              t) { m_stream.ts << t; return *this; }
    LogStream &operator<<(const wchar_t*      t) { m_stream.ts << t; return *this; }
    LogStream &operator<<(const std::wstring& t) { m_stream.ts << t; return *this; }
    LogStream &operator<<(const void*         t) { m_stream.ts << t; return *this; }
    typedef std::ios_base& (__CLRCALL_OR_CDECL *IosBaseFunc)(std::ios_base& _Iosbase);
    LogStream &operator<<(const IosBaseFunc   t) { m_stream.ts << t; return *this; }

  private:
    struct Stream {
      Stream(ILogger* logger, const ILogger::LogLevel level, const char* file, int line, const char* funcName)
        : m_logger(logger)
        , m_level(level)
        , m_file(file)
        , m_line(line)
        , m_funcName(funcName)
      {
      }
      ~Stream() {
        auto str = ts.str();
        auto msg = str.c_str();
        switch (m_level) {
        case ILogger::LogLevel::Trace:  m_logger->trace(msg, m_file, m_line, m_funcName); break;
        case ILogger::LogLevel::Info :  m_logger->info (msg, m_file, m_line, m_funcName); break;
        case ILogger::LogLevel::Debug:  m_logger->debug(msg, m_file, m_line, m_funcName); break;
        case ILogger::LogLevel::Warn :  m_logger->warn (msg, m_file, m_line, m_funcName); break;
        case ILogger::LogLevel::Error:  m_logger->error(msg, m_file, m_line, m_funcName); break;
        case ILogger::LogLevel::Fatal:  m_logger->fatal(msg, m_file, m_line, m_funcName); break;
        }
      }
      std::wostringstream ts;
      const ILogger::LogLevel m_level;
      ILogger* m_logger;
      const char* m_file;
      int m_line;
      const char* m_funcName;
    };
    Stream m_stream;
  };
}

#define SERGZ_LOG(logger, _level, file, line, funcName) \
  if (nullptr == logger) ; \
    else sergz::LogStream(logger, _level, file, line, funcName)

#define SERGZ_TRACE_ON(logger) SERGZ_LOG(logger, sergz::ILogger::LogLevel::Trace, __FILE__, __LINE__, __FUNCSIG__)
#define SERGZ_INFO_ON(logger)  SERGZ_LOG(logger, sergz::ILogger::LogLevel::Info , __FILE__, __LINE__, __FUNCSIG__)
#define SERGZ_DEBUG_ON(logger) SERGZ_LOG(logger, sergz::ILogger::LogLevel::Debug, __FILE__, __LINE__, __FUNCSIG__)
#define SERGZ_WARN_ON(logger)  SERGZ_LOG(logger, sergz::ILogger::LogLevel::Warn , __FILE__, __LINE__, __FUNCSIG__)
#define SERGZ_ERROR_ON(logger) SERGZ_LOG(logger, sergz::ILogger::LogLevel::Error, __FILE__, __LINE__, __FUNCSIG__)
#define SERGZ_FATAL_ON(logger) SERGZ_LOG(logger, sergz::ILogger::LogLevel::Fatal, __FILE__, __LINE__, __FUNCSIG__)

#ifdef _DEBUG
#define SERGZ_TRACE() SERGZ_TRACE_ON(m_logger.get())
#define SERGZ_INFO()  SERGZ_INFO_ON(m_logger.get())
#define SERGZ_DEBUG() SERGZ_DEBUG_ON(m_logger.get())
#define SERGZ_WARN()  SERGZ_WARN_ON(m_logger.get())
#define SERGZ_ERROR() SERGZ_ERROR_ON(m_logger.get())
#define SERGZ_FATAL() SERGZ_FATAL_ON(m_logger.get())
#else
#define SERGZ_LOG_DISABLED if (true); else sergz::LogStream(nullptr, sergz::ILogger::LogLevel::Fatal, "", 0, "")
#define SERGZ_TRACE() SERGZ_LOG_DISABLED
#define SERGZ_INFO()  SERGZ_LOG_DISABLED
#define SERGZ_DEBUG() SERGZ_LOG_DISABLED
#define SERGZ_WARN()  SERGZ_LOG_DISABLED
#define SERGZ_ERROR() SERGZ_LOG_DISABLED
#define SERGZ_FATAL() SERGZ_LOG_DISABLED
#endif