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
#include <cstdint>
#include <windef.h>

namespace ukot { namespace utils {
  class DpiAwareness {
  protected:
    uint32_t DPIAware(uint32_t value) const {
      return value * m_dpi / 96;
    }
    SIZE DPIAware(SIZE value) const {
      return SIZE{ DPIAware(value.cx), DPIAware(value.cy) };
    }
    RECT DPIAware(RECT value) const {
      return RECT{ DPIAware(value.left), DPIAware(value.top), DPIAware(value.right), DPIAware(value.bottom) };
    }
    uint32_t m_dpi = 96;
  };
}}