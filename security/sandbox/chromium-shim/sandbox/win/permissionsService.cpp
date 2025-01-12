/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/* SandboxPermissions.cpp - Special permissions granted to sandboxed processes */

#include "permissionsService.h"
#include <algorithm>
#include <winternl.h>

namespace mozilla {
namespace sandboxing {

static const std::wstring ZONE_IDENTIFIER_STR(L":ZONE.IDENTIFIER");
static const std::wstring ZONE_ID_DATA_STR(L":ZONE.IDENTIFIER:$DATA");

bool
StringEndsWith(const std::wstring& str, const std::wstring& strEnding)
{
  if (strEnding.size() > str.size()) {
    return false;
  }
  return std::equal(strEnding.rbegin(), strEnding.rend(), str.rbegin());
}

// Converts NT device internal filenames to normal user-space by stripping
// the prefixes and suffixes from the file name.
std::wstring
GetPlainFileName(const wchar_t* aNTFileName)
{
  while (*aNTFileName == L'\\' || *aNTFileName == L'.' ||
         *aNTFileName == L'?' || *aNTFileName == L':' ) {
    ++aNTFileName;
  }
  std::wstring nameCopy(aNTFileName);
  std::transform(nameCopy.begin(), nameCopy.end(), nameCopy.begin(), towupper);
  if (StringEndsWith(nameCopy, ZONE_ID_DATA_STR)) {
    nameCopy = nameCopy.substr(0, nameCopy.size() - ZONE_ID_DATA_STR.size());
  } else if (StringEndsWith(nameCopy, ZONE_IDENTIFIER_STR)) {
    nameCopy = nameCopy.substr(0, nameCopy.size() - ZONE_IDENTIFIER_STR.size());
  }
  return nameCopy;
}

/* static */ PermissionsService*
PermissionsService::GetInstance()
{
  static PermissionsService sPermissionsService;
  return &sPermissionsService;
}

PermissionsService::PermissionsService() :
  mFileAccessViolationFunc(nullptr)
{
}

void
PermissionsService::GrantFileAccess(uint32_t aProcessId,
                                    const wchar_t* aFilename,
                                    bool aPermitWrite)
{
  FilePermissionMap& permissions = mProcessFilePermissions[aProcessId];
  std::wstring filename = GetPlainFileName(aFilename);
  permissions[filename] |= aPermitWrite;
}

void
PermissionsService::SetFileAccessViolationFunc(FileAccessViolationFunc aFavFunc)
{
  mFileAccessViolationFunc = aFavFunc;
}

void
PermissionsService::ReportBlockedFile(bool aNeedsWrite)
{
  if (mFileAccessViolationFunc) {
    mFileAccessViolationFunc(aNeedsWrite);
  }
}

bool
PermissionsService::UserGrantedFileAccess(uint32_t aProcessId,
                                          const wchar_t* aFilename,
                                          uint32_t aAccess,
                                          uint32_t aDisposition)
{
  // There are 3 types of permissions:
  // * Those available w/ read-only permission
  // * Those available w/ read-only AND read-write permission
  // * Those always forbidden.
  const uint32_t FORBIDDEN_FLAGS =
    FILE_EXECUTE | FILE_LIST_DIRECTORY | FILE_TRAVERSE | STANDARD_RIGHTS_EXECUTE;
  const uint32_t NEEDS_WRITE_FLAGS =
    FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA | FILE_APPEND_DATA |
    DELETE | STANDARD_RIGHTS_WRITE;
  bool needsWrite =
    (aAccess & NEEDS_WRITE_FLAGS) || (aDisposition != FILE_OPEN);

  if (aAccess & FORBIDDEN_FLAGS) {
    ReportBlockedFile(needsWrite);
    return false;
  }

  auto permissions = mProcessFilePermissions.find(aProcessId);
  if (permissions == mProcessFilePermissions.end()) {
    ReportBlockedFile(needsWrite);
    return false;  // process has no special file access at all
  }

  std::wstring filename = GetPlainFileName(aFilename);
  auto itPermission = permissions->second.find(filename);
  if (itPermission == permissions->second.end()) {
    ReportBlockedFile(needsWrite);
    return false;  // process has no access to this file
  }

  // We have read permission.  Check for write permission if requested.
  if (!needsWrite || itPermission->second) {
    return true;
  }

  // We needed write access but didn't have it.
  ReportBlockedFile(needsWrite);
  return false;
}

void
PermissionsService::RemovePermissionsForProcess(uint32_t aProcessId)
{
  mProcessFilePermissions.erase(aProcessId);
}

}   // namespace sandboxing
}   // namespace mozilla
