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
#include "LoggerFactoryImpl.h"
#include "LoggerImpl.h"

#include <log4cplus/logger.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/layout.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/configurator.h>
#include <log4cplus/win32debugappender.h>

#include <cassert>
#include "../include/sergz.logger/createLoggerFactory.h"

using ukot::logger::LoggerFactoryImpl;
using ukot::logger::LoggerImpl;

std::shared_ptr<ukot::ILoggerFactory> ukot::createLoggerFactory() {
  return std::make_shared<LoggerFactoryImpl>();
}

LoggerFactoryImpl::LoggerFactoryImpl()
{
  ::log4cplus::initialize();
  ::log4cplus::BasicConfigurator config;
  config.configure();
  ::log4cplus::Logger::getRoot().setLogLevel(::log4cplus::ALL_LOG_LEVEL);
  ::log4cplus::Logger::getRoot().removeAllAppenders();
  ::log4cplus::SharedAppenderPtr win32DebugAppender(new ::log4cplus::Win32DebugAppender());
  ::log4cplus::tstring pattern = LOG4CPLUS_TEXT("%-5p %c %r - %m%n");
  // %n - new line
  // %c - logger name 
  // %p - level
  // %r Used to output the number of milliseconds elapsed since the start of the application until the creation of the logging event.
  // %m - message

  win32DebugAppender->setLayout(std::auto_ptr< ::log4cplus::Layout>(new ::log4cplus::PatternLayout(pattern)));
  ::log4cplus::Logger::getRoot().addAppender(win32DebugAppender);
}

LoggerFactoryImpl::~LoggerFactoryImpl() {}

void LoggerFactoryImpl::propertiesFile(const std::wstring& propertiesFile, const Properties& userProperties) {
  assert(!propertiesFile.empty() && "properties file path cannot be empty");
  if (propertiesFile.empty()) {
    return;
  }

  ::log4cplus::helpers::Properties properties(propertiesFile);
  for (const auto& userProperty : userProperties) {
    properties.setProperty(userProperty.first, userProperty.second);
  }
  ::log4cplus::PropertyConfigurator configurator(properties, ::log4cplus::Logger::getDefaultHierarchy(), ::log4cplus::PropertyConfigurator::fShadowEnvironment);
  configurator.configure();
}

std::unique_ptr<ukot::ILogger> LoggerFactoryImpl::getLogger(const wchar_t* name) {
  assert(name && "No name given for logger");
  if (nullptr == name) {
    return nullptr;
  }

  return std::make_unique<LoggerImpl>(name);
}