/***************************************************************************************************
 *  Copyright 2013 MaidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of MaidSafe.net.                                  *
 **************************************************************************************************/

#ifndef MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HOLDER_H_
#define MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HOLDER_H_

#include <cstdint>
#include <utility>

#include "maidsafe/common/types.h"
#include "maidsafe/nfs/types.h"


namespace maidsafe {

namespace nfs {

template<>
struct PersonaTypes<Persona::kMaidAccountHolder> {
  typedef DataNameVariant DbKey;
  typedef int32_t DbValue;
  typedef std::pair<DbKey, MessageAction> UnresolvedEntryKey;
  typedef DbValue UnresolvedEntryValue;
  static const Persona persona = Persona::kMaidAccountHolder;
};

}  // namespace nfs


namespace vault {

typedef nfs::PersonaTypes<nfs::Persona::kMaidAccountHolder> MaidAccountHolder;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HOLDER_H_
