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
#include "LoggerImpl.h"
#include "LoggerFactoryImpl.h"
#include <log4cplus/loggingmacros.h>

using ukot::logger::LoggerImpl;

LoggerImpl::LoggerImpl(const std::wstring& name)
  : m_name(name)
{
  m_logger = std::make_unique<::log4cplus::Logger>(::log4cplus::Logger::getInstance(m_name));
}

LoggerImpl::~LoggerImpl() {}

namespace {
  ::log4cplus::LogLevel mapLogLevel(ukot::ILogger::LogLevel level) {
    using ukot::ILogger;
    switch (level) {
    case ILogger::LogLevel::Fatal:
      return ::log4cplus::FATAL_LOG_LEVEL;
    case ILogger::LogLevel::Error:
      return ::log4cplus::ERROR_LOG_LEVEL;
    case ILogger::LogLevel::Warn:
      return ::log4cplus::WARN_LOG_LEVEL;
    case ILogger::LogLevel::Debug:
      return ::log4cplus::DEBUG_LOG_LEVEL;
    case ILogger::LogLevel::Info:
      return ::log4cplus::INFO_LOG_LEVEL;
    case ILogger::LogLevel::Trace:
      return ::log4cplus::TRACE_LOG_LEVEL;
    default:
      assert(false && "Invalid loglevel");
    }
    return ::log4cplus::TRACE_LOG_LEVEL;
  }
}

void LoggerImpl::log(
  LogLevel level,
  const wchar_t* value,
  const char* fileName /* = nullptr */,
  int line /* = 0 */,
  const char* funcName /* = nullptr */)
{
  auto log4cplusLevel = mapLogLevel(level);
  if (!m_logger || !m_logger->isEnabledFor(log4cplusLevel)) {
    return;
  }
  ::log4cplus::detail::macro_forced_log(*m_logger,
    log4cplusLevel,
    value ? value : L"",
    fileName ? fileName : "",
    line,
    funcName ? funcName : "");
}
