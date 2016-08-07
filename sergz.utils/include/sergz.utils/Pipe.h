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
#include "ScopedHandle.h"
#include "IOCompletionPortOverlapped.h"
#include <memory>
#include <string>
#include <array>
#include <vector>
#include <map>

namespace ukot { namespace utils {
  class Pipe: Noncopyable {
    struct PrivateCtrArg{};
  public:
    typedef std::unique_ptr<Pipe> pointer;
    struct Params {
      std::wstring name;
      std::function<void()> disconnected;
      std::function<void(const void* data, size_t dataLength)> dataRead;
    };
    struct WriteOverlapped;
    explicit Pipe(const Params& params, PrivateCtrArg);
    ~Pipe();
    static pointer createServer(const Params& params);
    static pointer connectTo(const Params& params);
    void readAsync();
    bool writeAsync(const std::vector<uint8_t>& value, const std::function<void()>& onWritten = std::function<void()>());
    HANDLE handle() {
      return m_pipe.handle();
    }
  private:
    void writeNextPartAsync(WriteOverlapped& writeInfo);
    // it's not in anonymous namespace to give the function the access to private members
    static VOID CALLBACK readCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped);
    void disconnected();
    void dataRead(const void* data, size_t dataLength);
    static void CALLBACK writeCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered, OVERLAPPED* lpOverlapped);
  private:
    struct ReadOverlapped : IOCompletionPortOverlapped {
      explicit ReadOverlapped(Pipe* parent)
        : m_parent(parent)
      {
        func = [this]{
          std::string x;
        };
      }
      Pipe* m_parent;
      std::array<uint8_t, 1024> m_buffer;
    };
    FileHandle m_pipe;
    ReadOverlapped m_readOverlapped;
    Params m_params;
    std::vector<std::unique_ptr<WriteOverlapped>> m_toWriteData;
    uint64_t m_messageIdGenerator;
    std::map<uint64_t, std::vector<uint8_t>> m_incompleteReadMessages;
  };

  struct PipeHolder {
    ukot::utils::Pipe::pointer pipe;
  };
  typedef std::shared_ptr<PipeHolder> PipeHolderPtr;
}}