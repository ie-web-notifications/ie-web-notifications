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

#ifndef STRICT
#define STRICT
#endif

#include "targetver.h"

#ifdef _DEBUG
//#define _ATL_DEBUG_QI
//#define _ATL_DEBUG_INTERFACES
#endif

// This define is very useful for release, it significantly reduces the output size
// Don't forget to remove the reference
#define SERGZ_LOGGER_DISABLED 1

#include <sergz.atl/pch-common.h>
#include <sergz.atl/atl.h>

#include "resource.h"
#include <cassert>
#include <sergz.logger/Logger.h>
