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
#include <cassert>
#include "../include/sergz.utils/EventWithSetter.h"

using namespace ukot::utils;

EventWithSetter::Setter::Setter(const DataPtr& data)
  : m_data(data)
{
}

EventWithSetter::Setter::~Setter()
{
  if (!m_data->setCalled)
  {
    std::lock_guard<std::mutex> lock(m_data->mutex);
    m_data->value = true;
    m_data->event.notify_one();
  }
}

void EventWithSetter::Setter::set()
{
  m_data->setCalled = true;
  std::lock_guard<std::mutex> lock(m_data->mutex);
  m_data->value = true;
  m_data->event.notify_one();
}

EventWithSetter::EventWithSetter()
  : m_data(std::make_shared<Data>())
{
}

std::shared_ptr<EventWithSetter::Setter> EventWithSetter::getSetter() {
  std::shared_ptr<Setter> retValue;
  assert(!m_data->setterCreated && "Unexpected second call of GetSetter");
  if (m_data->setterCreated) {
    return retValue;
  }
  retValue = std::make_shared<Setter>(m_data);
  m_data->setterCreated = true;
  return retValue;
}

bool EventWithSetter::wait(std::chrono::milliseconds timeout) {
  assert(m_data->setterCreated && "Wrong usage: the setter should be created"
    " before calling Wait.");
  std::unique_lock<std::mutex> lock(m_data->mutex);
  auto waitPredicate = [this]()->bool
  {
    return m_data->value;
  };
  return m_data->setterCreated
    && m_data->event.wait_for(lock, timeout, waitPredicate)
    && m_data->setCalled;
}