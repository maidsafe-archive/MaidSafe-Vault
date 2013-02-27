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

#ifndef MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_SYNC_H_
#define MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_SYNC_H_

#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/maid_account_holder/maid_account.h"

namespace maidsafe {

namespace vault {

class MaidAccountSync {
 public:

  std::vector<boost::filesystem::path> AddSyncInfoUpdate(
    const NodeId& node_id,
    const MaidAccount::serialised_info_type& serialised_account_info,
    const Accumulator<passport::PublicMaid::name_type>::serialised_requests& serialised_handled_request);  // NOLINT

  void AddDownloadedFile(boost::filesystem::path file_name, const NonEmptyString& file_contents);

  std::vector<boost::filesystem::path> GetFileRequests(const NodeId& node_id);

  bool MergeSyncResults(std::unique_ptr<MaidAccount>& account, Accumulator<MaidName>& accumulator);

 private:
  struct SyncInfoUpdate {
    NodeId node_id;
    MaidAccount::AccountInfo account_info;
    std::vector<Accumulator<passport::PublicMaid::name_type>::HandledRequest> handled_requests;
    std::vector<boost::filesystem::path> shared_file_names, requested_file_names;
  };

  const MaidName kMaidName_;
  std::vector<boost::filesystem::path> downloaded_files_;
  std::vector<SyncInfoUpdate> sync_updates_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_SYNC_H_
