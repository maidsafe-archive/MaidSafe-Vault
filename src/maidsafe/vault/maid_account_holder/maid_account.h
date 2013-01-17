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

#ifndef MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_H_
#define MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_H_

#include <vector>
#include <mutex>
#include <functional>

#include "maidsafe/common/types.h"

#include "maidsafe/vault/types.h"
#include "maidsafe/vault/maid_account_holder/maid_account_pb.h"


namespace maidsafe {

namespace vault {

class MaidAccount {
 public:
  struct {   }structure;
  typedef MaidName name_type;
  explicit MaidAccount(const MaidName& maid_name);
  explicit MaidAccount(const NonEmptyString& serialised_maid_account);
  NonEmptyString Serialise() const;

  void Add(const protobuf::PmidTotals& pmid_totals);
  void Remove(const PmidName& pmid_name);
  void Update(const protobuf::PmidTotals& pmid_total);
  template<typename Data>
  bool ModifyOrAddDataElement(const typename Data::name_type& name,
                              int32_t version,
                              const structure& serialised_value,
                              std::function<void(std::string&)> modify_functor);
  void DeleteDataElement(const typename Data::name_type& name,
                         int32_t version);

  MaidName maid_name() const { return kMaidName_; }

 private:
  protobuf::MaidAccount proto_maid_account_;
  const MaidName kMaidName_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_H_
