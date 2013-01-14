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

#include "maidsafe/vault/maid_account_holder/maid_account_handler.h"

namespace maidsafe {

namespace vault {

const boost::filesystem::path kMaidAccountsName("maid_accounts");

MaidAccountHandler::MaidAccountHandler(const boost::filesystem::path& vault_root_dir)
    : maid_accounts_path_(vault_root_dir / kMaidAccountsName),
      maid_pmid_info_(),
      maid_storage_fifo_(),
      active_() {}

void MaidAccountHandler::AddDataElement(const MaidName& maid_name, const protobuf::PutData& data) {
  int index(-1);
  auto data_element =
      std::find_if(maid_storage_fifo_.begin(),
                   maid_storage_fifo_.end(),
                   [&index, &maid_name, &data, this]
                   (const protobuf::MaidAccountStorage& maid_storage) {
                     return MatchMaidStorageFifoEntry(maid_storage,
                                                      maid_name,
                                                      data,
                                                      std::ref(index));
                   });
  if (data_element == maid_storage_fifo_.end() && index > -1) {
    // Look in the files
  } else {
    (*data_element).mutable_recent_put_data(index)->CopyFrom(data);
    // Store to disk
  }
}

bool MaidAccountHandler::MatchMaidStorageFifoEntry(const protobuf::MaidAccountStorage& maid_storage,
                                                   const MaidName& maid_name,
                                                   const protobuf::PutData& data,
                                                   int &index) {
  if (maid_storage.maid_name() != maid_name.data.string())
    return false;

  for (int n(0); n != maid_storage.recent_put_data_size(); ++n) {
    if (maid_storage.recent_put_data(n).data_element().name() == data.data_element().name() &&
        maid_storage.recent_put_data(n).data_element().version() == data.data_element().version()) {
      index = n;
      return true;
    }
  }

  return false;
}


}  // namespace vault

}  // namespace maidsafe
