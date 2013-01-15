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
const int kMaxFileElementSize(1000);

MaidAccountHandler::MaidAcountingFileInfo::MaidAcountingFileInfo()
  : maid_name(),
    element_count(0),
    current_file(0) {}

MaidAccountHandler::MaidAcountingFileInfo::MaidAcountingFileInfo(const MaidName& maid_name_in,
                                                                 int element_count_in,
                                                                 int current_file_in)
    : maid_name(maid_name_in),
      element_count(element_count_in),
      current_file(current_file_in) {}

MaidAccountHandler::MaidAccountHandler(const boost::filesystem::path& vault_root_dir)
    : maid_accounts_path_(vault_root_dir / kMaidAccountsName),
      maid_pmid_info_(),
      maid_storage_fifo_(),
      acounting_file_info_(),
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
    active_.Send([=] () { IncrementDuplicationAndStoreToFile(maid_name, data); });
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

void MaidAccountHandler::IncrementDuplicationAndStoreToFile(const MaidName& maid_name,
                                                            const protobuf::PutData& data) {
  auto accounting_info_it(std::find_if(acounting_file_info_.begin(),
                                       acounting_file_info_.end(),
                                       [&maid_name] (const MaidAcountingFileInfo& file_info) {
                                         return file_info.maid_name == maid_name;
                                       }));
  if (accounting_info_it == acounting_file_info_.end()) {
    LOG(kError) << "We seem to think we have the account but no record of files: "
                << Base64Substr(maid_name.data);
    // TODO(Team): We might be able to by-pass or recover from this error. Leaving exception as
    //             to debug since it shouldn't really happen.
    throw std::exception();
  }

  if ((*accounting_info_it).element_count == kMaxFileElementSize) {
    LOG(kInfo) << "Need new file: " << Base64Substr(maid_name.data);
    (*accounting_info_it).element_count = 1;
    ++(*accounting_info_it).current_file;
  }

  // Find the entry to the account
  boost::filesystem::path file_path(
      maid_accounts_path_ / (EncodeToBase64(maid_name.data) + "." +
                             boost::lexical_cast<std::string>((*accounting_info_it).current_file)));
  NonEmptyString current_content(ReadFile(file_path));
  protobuf::ArchivedData archived_data;
  if (!archived_data.ParseFromString(current_content.string())) {
    LOG(kError) << "Failed to parse. Data could be corrupted.";
    throw std::exception();
  }

  for (int n(0); n != archived_data.put_data_size(); ++n) {
    if (archived_data.put_data(n).data_element().name() == data.data_element().name() &&
        archived_data.put_data(n).data_element().version() == data.data_element().version()) {
      archived_data.mutable_put_data(n)->set_replications(archived_data.put_data(n).replications());
      if (!WriteFile(file_path, archived_data.SerializeAsString())) {
        LOG(kError) << "Failed to write file after increasing replication: " << file_path;
        throw std::exception();
      } else {
        n = archived_data.put_data_size();
      }
    }
  }
}

}  // namespace vault

}  // namespace maidsafe
