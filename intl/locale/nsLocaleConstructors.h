/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef nsLocaleConstructors_h__
#define nsLocaleConstructors_h__

#include "nsCollationCID.h"
#include "mozilla/ModuleUtils.h"
#include "nsILocaleService.h"
#include "nsIScriptableDateFormat.h"
#include "nsIServiceManager.h"
#include "nsLanguageAtomService.h"
#include "nsPlatformCharset.h"
#include "LocaleService.h"

#if defined(XP_MACOSX)
#define USE_MAC_LOCALE
#endif

#if defined(XP_UNIX) && !defined(XP_MACOSX)
#define USE_UNIX_LOCALE
#endif

#ifdef XP_WIN
#include "windows/nsCollationWin.h"
#endif

#ifdef USE_MAC_LOCALE
#include "mac/nsCollationMacUC.h"
#endif

#ifdef USE_UNIX_LOCALE
#include "unix/nsCollationUnix.h"
#endif

#define NSLOCALE_MAKE_CTOR(ctor_, iface_, func_)          \
static nsresult                                           \
ctor_(nsISupports* aOuter, REFNSIID aIID, void** aResult) \
{                                                         \
  *aResult = nullptr;                                      \
  if (aOuter)                                             \
    return NS_ERROR_NO_AGGREGATION;                       \
  iface_* inst;                                           \
  nsresult rv = func_(&inst);                             \
  if (NS_SUCCEEDED(rv)) {                                 \
    rv = inst->QueryInterface(aIID, aResult);             \
    NS_RELEASE(inst);                                     \
  }                                                       \
  return rv;                                              \
}


NSLOCALE_MAKE_CTOR(CreateLocaleService, nsILocaleService, NS_NewLocaleService)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsCollationFactory)
//NS_GENERIC_FACTORY_CONSTRUCTOR(nsScriptableDateTimeFormat)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLanguageAtomService)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPlatformCharset, Init)

namespace mozilla {
namespace intl {
NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(LocaleService,
                                         LocaleService::GetInstanceAddRefed)
}
}

#ifdef XP_WIN
NS_GENERIC_FACTORY_CONSTRUCTOR(nsCollationWin)
#endif

#ifdef USE_UNIX_LOCALE
NS_GENERIC_FACTORY_CONSTRUCTOR(nsCollationUnix)
#endif  

#ifdef USE_MAC_LOCALE
NS_GENERIC_FACTORY_CONSTRUCTOR(nsCollationMacUC)
#endif  

#endif
