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

#ifndef MAIDSAFE_VAULT_MAID_ACCOUNT_MANAGER_H_
#define MAIDSAFE_VAULT_MAID_ACCOUNT_MANAGER_H_

#include <future>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/active.h"

#include "maidsafe/vault/maid_account_holder/maid_account_pb.h"
#include "maidsafe/vault/types.h"

namespace maidsafe {

namespace vault {

class MaidAccountHandler {
 public:
  MaidAccountHandler(const boost::filesystem::path& vault_root_dir);

  // Data operations
  void AddDataElement(const MaidName& maid_name, const protobuf::PutData& data);
  void UpdateReplicationCount(const MaidName& maid_name,
                              const protobuf::PutData& data,
                              int32_t replications);
  void DeleteDataElement(const MaidName& maid_name, const protobuf::PutData& data);
  // Optional
  // void GetDataElement(protobuf::MaidAccountStorage& storage_element_with_name);
  // int32_t GetDuplicates(const MaidName& maid_name);

  // PmidInfo operations
  void AddPmidToAccount(const protobuf::MaidPmidsInfo& new_pmid_for_maid);
  void RemovePmidFromAccount(const MaidName& maid_name, const PmidName& pmid_name);
  void GetMaidAccountTotals(protobuf::MaidPmidsInfo& info_with_maid_name) const;
  void UpdatePmidTotals(const MaidName& maid_name, const protobuf::PmidTotals& pmid_totals);

  // Sync operations
  std::vector<MaidName> GetMaidNames() const;
  size_t GetMaidAccountFileCount() const;
  std::future<std::string> GetMaidAccountFile(size_t index) const;
  // Optional
  // void GetPmidAccountDetails(protobuf::PmidRecord& pmid_record);

 private:
  std::vector<protobuf::MaidPmidsInfo> maid_pmid_info_;
  Active active_;
  // File handler extracted from template version
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_MANAGER_H_
