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
#include <winstring.h>
#include <string>

namespace ukot { namespace ie_web_notifications {
  // These functions are taken from some example from Microsoft, can be found on MSDN.

  // Warning: The caller must ensure the lifetime of the buffer outlives this      
  // object as it does not make a copy of the wide string memory.      
  class StringReferenceWrapper
  {
  public:
    // Constructor which takes an existing string buffer and its length as the parameters.
    // It fills an HSTRING_HEADER struct with the parameter.      
    StringReferenceWrapper(_In_reads_(length) PCWSTR stringRef, _In_ UINT32 length) throw()
    {
      HRESULT hr = WindowsCreateStringReference(stringRef, length, &_header, &_hstring);

      if (FAILED(hr))
      {
        RaiseException(static_cast<DWORD>(STATUS_INVALID_PARAMETER), EXCEPTION_NONCONTINUABLE, 0, nullptr);
      }
    }
    explicit StringReferenceWrapper(const std::wstring& value) throw()
    {
      HRESULT hr = WindowsCreateStringReference(value.c_str(), static_cast<UINT32>(value.length()), &_header, &_hstring);

      if (FAILED(hr))
      {
        RaiseException(static_cast<DWORD>(STATUS_INVALID_PARAMETER), EXCEPTION_NONCONTINUABLE, 0, nullptr);
      }
    }

    ~StringReferenceWrapper()
    {
      WindowsDeleteString(_hstring);
    }

    template <size_t N>
    explicit StringReferenceWrapper(_In_reads_(N) wchar_t const (&stringRef)[N]) throw()
    {
      UINT32 length = N - 1;
      HRESULT hr = WindowsCreateStringReference(stringRef, length, &_header, &_hstring);

      if (FAILED(hr))
      {
        RaiseException(static_cast<DWORD>(STATUS_INVALID_PARAMETER), EXCEPTION_NONCONTINUABLE, 0, nullptr);
      }
    }

    HSTRING Get() const throw()
    {
      return _hstring;
    }

  private:
    HSTRING             _hstring;
    HSTRING_HEADER      _header;
  };
}}