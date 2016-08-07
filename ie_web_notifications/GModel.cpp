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
#include "GModel.h"
#include "ClientImpl.h"

using namespace ukot::ie_web_notifications;

GModel& GModel::instance() {
  static GModel model;
  return model;
}

GModel::GModel()
{
}

GModel::~GModel() {
}

ClientPtr GModel::getClient() {
  ClientPtr retValue;
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (auto client = m_client.lock()) {
      return client;
    }
    m_client = retValue = std::make_shared<ClientImpl>();
  }
  return  retValue;
}
