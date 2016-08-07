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
#include "../include/sergz.utils/Pipe.h"
#include <array>
#include <aclapi.h>
#include <sddl.h>

using namespace ukot::utils;

namespace {
  const DWORD g_kBufferSize = 1024;
}

namespace {
  // security crazy stuff
  // http://msdn.microsoft.com/en-us/library/windows/desktop/hh448493(v=vs.85).aspx

  template<typename Structure>
  struct StructureOnMemory
  {
    StructureOnMemory(uint32_t length)
      : memory{new uint8_t[length]}
      , value{reinterpret_cast<Structure*>(memory.get())}
    {
    }
  private:
    std::unique_ptr<uint8_t[]> memory;
  public:
    Structure* value;
  };

  typedef StructureOnMemory<SID> Sid;
  typedef std::unique_ptr<Sid> SidPtr;
  SidPtr getLogonSid(HANDLE token)
  {
    DWORD tokenGroupsLength = 0;
    if (GetTokenInformation(token, TokenLogonSid, 0, 0, &tokenGroupsLength)) {
      // it's unexpected because we are quering for the length
      return SidPtr{};
    }
    {
      DWORD lastError = GetLastError();
      if (lastError != ERROR_INSUFFICIENT_BUFFER) {
        // if there is another error then return it
        return SidPtr{};
      }
    }

    std::unique_ptr<uint8_t[]> tokenGroupsMemory(new uint8_t[tokenGroupsLength]);
    auto tokenGroups = reinterpret_cast<TOKEN_GROUPS*>(tokenGroupsMemory.get());
    if (!GetTokenInformation(token, TokenLogonSid, tokenGroups, tokenGroupsLength, &tokenGroupsLength)) {
      DWORD lastError = GetLastError();
      return SidPtr{};
    }
    if (tokenGroups->GroupCount != 1) {
      DWORD lastError = GetLastError();
      return SidPtr{};
    }

    if (!IsValidSid(tokenGroups->Groups[0].Sid)) {
      auto lastErrorCode = GetLastError();
      return SidPtr{};
    }

    DWORD sidLength = GetLengthSid(tokenGroups->Groups[0].Sid);
    std::unique_ptr<Sid> retValue{new Sid{sidLength}};
    if (!CopySid(sidLength, retValue->value, tokenGroups->Groups[0].Sid)) {
      auto lastErrorCode = GetLastError();
      return SidPtr{};
    }
    return retValue;
  }

  // Creates a security descriptor: 
  // Allows ALL access to Logon SID and to all app containers in DACL.
  // Sets Low Integrity in SACL.
  struct SecurityDescriptor : ukot::utils::Noncopyable {
    typedef std::unique_ptr<SecurityDescriptor> SecurityDescriptorPtr;
    SECURITY_DESCRIPTOR* value;
    std::error_code errorCode;
    static SecurityDescriptorPtr create(PSID logonSid) {
      SecurityDescriptorPtr _this{ new SecurityDescriptor{} };
      _this->createImpl(logonSid);
      if (!!_this->errorCode)
      {
        _this->value = nullptr;
      }
      return _this;
    }
  private:
    void createImpl(PSID logonSid)
    {
      value = reinterpret_cast<SECURITY_DESCRIPTOR*>(valueMemory.data());
      if (!InitializeSecurityDescriptor(value, SECURITY_DESCRIPTOR_REVISION))
      {
        auto lastErrorCode = GetLastError();
        errorCode = std::error_code(lastErrorCode, std::system_category());
        return;
      }

      EXPLICIT_ACCESSW explicitAccess[2] = {};

      explicitAccess[0].grfAccessPermissions = STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL;
      explicitAccess[0].grfAccessMode = SET_ACCESS;
      explicitAccess[0].grfInheritance = NO_INHERITANCE;
      explicitAccess[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
      explicitAccess[0].Trustee.TrusteeType = TRUSTEE_IS_USER;
      explicitAccess[0].Trustee.ptstrName = static_cast<LPWSTR>(logonSid);

      // Create a well-known SID for the all appcontainers group.
      // We need to allow access to all AppContainers, since, apparently,
      // giving access to specific AppContainer (for example AppContainer of IE)
      // tricks Windows into thinking that token is IN AppContainer.
      // Which blocks all the calls from outside, making it impossible to communicate
      // with the engine when IE is launched with different security settings.
      PSID allAppContainersSid = 0;
      SID_IDENTIFIER_AUTHORITY applicationAuthority = SECURITY_APP_PACKAGE_AUTHORITY;

      AllocateAndInitializeSid(&applicationAuthority,
        SECURITY_BUILTIN_APP_PACKAGE_RID_COUNT,
        SECURITY_APP_PACKAGE_BASE_RID,
        SECURITY_BUILTIN_PACKAGE_ANY_PACKAGE,
        0, 0, 0, 0, 0, 0,
        &allAppContainersSid);
      std::unique_ptr<SID, decltype(&::FreeSid)> sharedAllAppContainersSid(static_cast<SID*>(allAppContainersSid), &FreeSid);

      explicitAccess[1].grfAccessPermissions = STANDARD_RIGHTS_ALL | SPECIFIC_RIGHTS_ALL;
      explicitAccess[1].grfAccessMode = SET_ACCESS;
      explicitAccess[1].grfInheritance = NO_INHERITANCE;
      explicitAccess[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
      explicitAccess[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
      explicitAccess[1].Trustee.ptstrName = static_cast<LPWSTR>(allAppContainersSid);

      PACL acl = 0;
      auto winErrorCode = SetEntriesInAcl(2, explicitAccess, 0, &acl);
      if (ERROR_SUCCESS != winErrorCode)
      {
        errorCode = std::error_code(winErrorCode, std::system_category());
        return;
      }
      sharedAclMemory = std::shared_ptr<void>(acl, &LocalFree);
      if (!SetSecurityDescriptorDacl(value, TRUE, acl, FALSE))
      {
        auto lastErrorCode = GetLastError();
        errorCode = std::error_code(lastErrorCode, std::system_category());
        return;
      }
      // Create a dummy security descriptor with low integrirty preset and copy its SACL into ours
      LPCWSTR accessControlEntry = L"S:(ML;;NW;;;LW)";
      PSECURITY_DESCRIPTOR dummySecurityDescriptorLow;
      if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(accessControlEntry, SDDL_REVISION_1, &dummySecurityDescriptorLow, 0))
      {
        auto lastErrorCode = GetLastError();
        errorCode = std::error_code(lastErrorCode, std::system_category());
        return;
      }
      dummySecurityDescriptorLowMemory = std::shared_ptr<void>(dummySecurityDescriptorLow, &LocalFree);
      BOOL saclPresent = FALSE;
      BOOL saclDefaulted = FALSE;
      PACL sacl;
      if (!GetSecurityDescriptorSacl(dummySecurityDescriptorLow, &saclPresent, &sacl, &saclDefaulted))
      {
        auto lastErrorCode = GetLastError();
        errorCode = std::error_code(lastErrorCode, std::system_category());
        return;
      }
      if (saclPresent)
      {
        if (!SetSecurityDescriptorSacl(value, TRUE, sacl, FALSE))
        {
          auto lastErrorCode = GetLastError();
          errorCode = std::error_code(lastErrorCode, std::system_category());
          return;
        }
      }
    }

    SecurityDescriptor()
      : value{nullptr}
    {
    }
  private:
    std::array<uint8_t, SECURITY_DESCRIPTOR_MIN_LENGTH> valueMemory;
    std::shared_ptr<void> sharedAclMemory;
    /// SACL is bound to this security descriptor, so we should keep it, while value is used.
    std::shared_ptr<void> dummySecurityDescriptorLowMemory;
  };
}

Pipe::pointer Pipe::createServer(const Params& params) {
  Pipe::pointer retValue = std::make_unique<Pipe>(params);
  DWORD openMode = PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED;
  DWORD pipeMode = PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT;
  DWORD maxInstances = PIPE_UNLIMITED_INSTANCES;
  SECURITY_ATTRIBUTES securityAttributes{};
  securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
  securityAttributes.bInheritHandle = FALSE;

  AccessToken token;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &token)) {
    DWORD lastError = GetLastError();
    return Pipe::pointer{};
  }
  auto logonSid = getLogonSid(token.handle());
  if (nullptr == logonSid) {
    return Pipe::pointer{};
  }
  std::unique_ptr<SecurityDescriptor> securityDescriptor = SecurityDescriptor::create(logonSid->value);
  if (nullptr == securityDescriptor) {
    return Pipe::pointer{};
  }
  securityAttributes.lpSecurityDescriptor = securityDescriptor->value;

  retValue->m_pipe = CreateNamedPipeW(params.name.c_str(), openMode, pipeMode, maxInstances,
    /*out buffer size*/ g_kBufferSize, /*in buffer size*/ g_kBufferSize, NMPWAIT_USE_DEFAULT_WAIT,
    /*security attribs*/ &securityAttributes);
  if (!retValue->m_pipe) {
    auto lastError = GetLastError();
    std::string x;
    return Pipe::pointer();
  }
  return retValue;
}

Pipe::pointer Pipe::connectTo(const Params& params) {
  if (0 == WaitNamedPipe(params.name.c_str(), 5000)) {
    DWORD lastError = GetLastError();
    std::string();
    return Pipe::pointer();
  }
  Pipe::pointer retValue = std::make_unique<Pipe>(params);
  DWORD desiredAccess = GENERIC_READ | GENERIC_WRITE;
  DWORD creationDisposition = OPEN_EXISTING;
  retValue->m_pipe = CreateFileW(params.name.c_str(), desiredAccess, /*dw shared model*/0,
    /*sec attr*/nullptr, creationDisposition, FILE_FLAG_OVERLAPPED,
    /*hTemplateFile*/ nullptr);
  if (!retValue->m_pipe) {
    auto lastError = GetLastError();
    std::string x;
    return Pipe::pointer();
  }
  DWORD pipeMode = PIPE_READMODE_MESSAGE;
  DWORD* maxCollectionCount = nullptr;
  DWORD* collectDataTimeout = nullptr;
  if (0 == SetNamedPipeHandleState(retValue->m_pipe.handle(), &pipeMode, maxCollectionCount, collectDataTimeout)) {
    DWORD lastError = GetLastError();
    std::string;
    return Pipe::pointer();
  }
  return retValue;
}

namespace {
#pragma pack(push, 1)
#pragma warning(push)
#pragma warning(disable:4200)
  enum class PipeMessagePacketStatus : uint8_t {
    nonTrailing, trailing
  };
  struct PipeMessagePacket {
    uint64_t messageId;
    PipeMessagePacketStatus status;
    uint16_t dataLength;
    char data[];
  };
#pragma warning(pop)
#pragma pack(pop)

  // advance() past end() results in undefined behaviour.
  template <class Iter, class Incr>
  void safe_advance(Iter& curr, const Iter& end, Incr n) {
    size_t remaining = distance(curr, end);
    if (remaining < n) {
      n = remaining;
    }
    advance(curr, n);
  }
}

struct Pipe::WriteOverlapped : OVERLAPPED{
  explicit WriteOverlapped(Pipe* parentArg, uint64_t messageIDArg, const std::vector<uint8_t>& dataArg)
    : OVERLAPPED{}
    , parent{ parentArg }
    , messageID(messageIDArg)
    , buffer(dataArg)
    , ii_nextPartBeginsAt(begin(buffer))
  {
  }
  Pipe* parent;
  const uint64_t messageID;
  const std::vector<uint8_t> buffer;
  std::vector<uint8_t>::const_iterator ii_nextPartBeginsAt;
  // it can be even more efficient if preappend it to buffer and move on each sending.
  std::vector<uint8_t> packet;
  std::function<void()> onWritten;
};

Pipe::Pipe(const Params& params, PrivateCtrArg = PrivateCtrArg{})
  : m_readOverlapped(this)
  , m_params(params)
  , m_messageIdGenerator(0)
{

}

Pipe::~Pipe() {

}

void Pipe::readAsync() {
  BOOL rc = ReadFileEx(m_pipe.handle(), m_readOverlapped.m_buffer.data(),
    m_readOverlapped.m_buffer.size(), &m_readOverlapped, &Pipe::readCompletionRoutine);
  if (0 == rc) {
    DWORD lastError = GetLastError();
    // it might should be queued
    disconnected();
  }
}

bool Pipe::writeAsync(const std::vector<uint8_t>& data, const std::function<void()>& onWritten) {
  assert(!data.empty() && "sending of only none empty data is supported");
  m_toWriteData.emplace_back(std::make_unique<utils::Pipe::WriteOverlapped>(this, ++m_messageIdGenerator, data));
  (*m_toWriteData.rbegin())->onWritten = onWritten;
  writeNextPartAsync(**m_toWriteData.rbegin());
  return true;
}

void Pipe::writeNextPartAsync(WriteOverlapped& writeInfo) {
  const auto ii_dataEnd = writeInfo.buffer.end();
  assert(g_kBufferSize > sizeof(PipeMessagePacket));
  const size_t maximumPacketMessageLength = g_kBufferSize - sizeof(PipeMessagePacket);
  auto payloadBeginsAt = writeInfo.ii_nextPartBeginsAt;
  safe_advance(writeInfo.ii_nextPartBeginsAt, ii_dataEnd, maximumPacketMessageLength);
  auto payloadDataLength = distance(payloadBeginsAt, writeInfo.ii_nextPartBeginsAt);
  writeInfo.packet.resize(sizeof(PipeMessagePacket) + payloadDataLength);
  auto packet = reinterpret_cast<PipeMessagePacket*>(writeInfo.packet.data());
  packet->messageId = writeInfo.messageID;
  packet->dataLength = payloadDataLength;
  bool isLastMessage = ii_dataEnd == writeInfo.ii_nextPartBeginsAt;
  packet->status = isLastMessage ? PipeMessagePacketStatus::trailing : PipeMessagePacketStatus::nonTrailing;
  memcpy(packet->data, &*payloadBeginsAt, packet->dataLength);
  WriteFileEx(handle(), writeInfo.packet.data(), writeInfo.packet.size(), &writeInfo, &writeCompletionRoutine);
}

VOID CALLBACK Pipe::readCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered,
  LPOVERLAPPED lpOverlapped) {
  auto readOverlapped = static_cast<ReadOverlapped*>(lpOverlapped);
  if (nullptr == readOverlapped) {
    // we cann't do anything, it's unexpected
    return;
  }
  if (ERROR_BROKEN_PIPE == dwErrorCode) {
    readOverlapped->m_parent->disconnected();
    return;
  } else if (ERROR_SUCCESS == dwErrorCode) {
    auto packet = reinterpret_cast<const PipeMessagePacket*>(readOverlapped->m_buffer.data());
    assert(sizeof(PipeMessagePacket) + packet->dataLength == dwNumberOfBytesTransfered);
    auto& map = readOverlapped->m_parent->m_incompleteReadMessages;
    auto ii_incompletetMessage = map.find(packet->messageId);
    if (map.end() == ii_incompletetMessage) {
      if (PipeMessagePacketStatus::trailing == packet->status) {
        readOverlapped->m_parent->dataRead(packet->data, packet->dataLength);
      } else {
        map.emplace(packet->messageId, std::vector<uint8_t>{packet->data, packet->data + packet->dataLength});
      }
    } else {
      ii_incompletetMessage->second.insert(ii_incompletetMessage->second.end(), packet->data, packet->data + packet->dataLength);
      if (PipeMessagePacketStatus::trailing == packet->status) {
        readOverlapped->m_parent->dataRead(ii_incompletetMessage->second.data(), ii_incompletetMessage->second.size());
        map.erase(ii_incompletetMessage);
      }
    }
    readOverlapped->m_parent->readAsync();
  }
}

void Pipe::disconnected() {
  CancelIo(m_pipe.handle());
  if (!!m_params.disconnected) {
    m_params.disconnected();
  }
}

void Pipe::dataRead(const void* data, size_t dataLength) {
  if (!!m_params.dataRead) {
    m_params.dataRead(data, dataLength);
  }
}

void CALLBACK Pipe::writeCompletionRoutine(DWORD dwErrorCode, DWORD dwNumberOfBytesTransfered,
  OVERLAPPED* lpOverlapped) {
  auto pWriteInfo = static_cast<WriteOverlapped*>(lpOverlapped);
  if (pWriteInfo->ii_nextPartBeginsAt != pWriteInfo->buffer.end()) {
    pWriteInfo->parent->writeNextPartAsync(*pWriteInfo);
    return;
  }
  auto& toWriteData = pWriteInfo->parent->m_toWriteData;
  auto ii_overlapepd = std::find_if(toWriteData.begin(), toWriteData.end(),
    [pWriteInfo](const std::unique_ptr<WriteOverlapped>& value) -> bool {
    return pWriteInfo == value.get();
  });
  if (pWriteInfo->onWritten) {
    pWriteInfo->onWritten();
  }
  if (toWriteData.end() != ii_overlapepd) {
    toWriteData.erase(ii_overlapepd);
  }
}