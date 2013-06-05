/***************************************************************************************************
 *  Copyright 2012 MaidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of MaidSafe.net.                                  *
 **************************************************************************************************/

#ifndef MAIDSAFE_VAULT_MPID_ACCOUNT_HOLDER_MPID_ACCOUNT_HOLDER_H_
#define MAIDSAFE_VAULT_MPID_ACCOUNT_HOLDER_MPID_ACCOUNT_HOLDER_H_

#include <utility>

#include "maidsafe/common/types.h"
#include "maidsafe/nfs/types.h"


namespace maidsafe {

namespace nfs {

template<>
struct PersonaTypes<Persona::kMpidAccountHolder> {
  typedef DataNameVariant DbKey;
  typedef NonEmptyString DbValue;
  typedef std::pair<DbKey, MessageAction> UnresolvedEntryKey;
  typedef DbValue UnresolvedEntryValue;
  static const Persona persona = Persona::kMpidAccountHolder;
};

}  // namespace nfs


namespace vault {

typedef nfs::PersonaTypes<nfs::Persona::kMpidAccountHolder> MpidAccountHolder;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MPID_ACCOUNT_HOLDER_MPID_ACCOUNT_HOLDER_H_
