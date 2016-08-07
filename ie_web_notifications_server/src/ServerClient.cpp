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

#include "ServerClient.h"
#include <ie_web_notifications.server.client/StartServer.h>

using namespace ukot::ie_web_notifications;

namespace {
  const wchar_t* g_kPipeName = L"\\\\.\\pipe\\ie_web_notification.server.pipe";
}

ServerClient::ServerClient()
{
  m_thread = std::thread{[this]{
    threadFunc();
  }};
}

ServerClient::~ServerClient() {
  m_exitThread = true;
  m_calls.push_front([]{}); // notify
  if (m_thread.joinable()){
    m_thread.join();
  }
}

void ServerClient::showSettings(const std::function<void()>& doneCallback) {
  m_calls.push_back([this, doneCallback]{
    auto isWritten = m_pipe->writeAsync(ShowSettingsPM::Create(), doneCallback);
    if (!isWritten) {
      // report about error
    }
  });
}

void ServerClient::threadFunc() {
  while (!m_exitThread) {
    switch (m_state) {
    case State::connecting:
      establishConnection();
      // if cannot establish connection try again but be polite
      if (State::connecting == m_state) {
        std::this_thread::yield();
      }
      break;
    case State::connected:
    {
      auto call = m_calls.pop_front();
      if (!!call) {
        call();
      }
    }
      break;
    default:
      ;
    }
  }
}

void ServerClient::establishConnection() {
  if (!startProcess()) {
    return;
  }
  if (!connectToPipe()) {
    return;
  }
  m_state = State::connected;
}

bool ServerClient::connectToPipe() {
  ukot::utils::Pipe::Params params;
  params.name = g_kPipeName;
  params.dataRead = [this](const void* data, size_t dataLength){
    onDataRead(data, dataLength);
  };
  m_pipe = ukot::utils::Pipe::connectTo(params);
  if (!m_pipe) {
    return false;
  }
  m_state = State::connected;
  m_pipe->readAsync();
  return true;
}

void ServerClient::onDataRead(const void* data, size_t /*data_length*/) {
  auto msg = static_cast<const BaseProtocolMessage*>(data);
  if (nullptr == msg) {
    return;
  }
  //TODO: call on read
}