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

#include <string>

namespace maidsafe {

namespace vault {

namespace {

boost::filesystem::path GetMaidAccountFileName(const MaidName& maid_name,
                                               const boost::filesystem::path& base_path,
                                               int file_count) {
  return base_path /
         (EncodeToBase64(maid_name.data) + "." + boost::lexical_cast<std::string>(file_count));
}

bool ArchivedElementEntryMatchesPutData(const protobuf::ArchivedData& archived_data,
                                        const protobuf::PutData& data,
                                        int index) {
  return archived_data.put_data(index).data_element().name() == data.data_element().name() &&
         archived_data.put_data(index).data_element().version() == data.data_element().version();
}

}  // namespace

const boost::filesystem::path kMaidAccountsName("maid_accounts");
const size_t kMaxElementSize(1000);
typedef std::vector<protobuf::MaidAccountStorage>::iterator MaidAccountStorageIterator;

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
      accounting_file_info_(),
      active_(),
      local_vectors_mutex_() {}

void MaidAccountHandler::AddDataElement(const MaidName& maid_name, const protobuf::PutData& data) {
  std::vector<MaidAcountingFileInfo>::iterator accounting_info_it;
  {
    std::lock_guard<std::mutex> guard(local_vectors_mutex_);
    FindAccountingEntry(maid_name, accounting_info_it);
    FindFifoEntryAndIncrement(maid_name, data);
  }
  active_.Send([=] () {
                 ActOnAccountFiles(maid_name, data, (*accounting_info_it).current_file);
               });
}

void MaidAccountHandler::UpdateReplicationCount(const MaidName& maid_name,
                                                const protobuf::PutData& data) {
  std::vector<MaidAcountingFileInfo>::iterator accounting_info_it;
  {
    std::lock_guard<std::mutex> guard(local_vectors_mutex_);
    FindAccountingEntry(maid_name, accounting_info_it);
    FindFifoEntryAndIncrement(maid_name, data);
  }

  active_.Send([=] () {
                 DoUpdateReplicationCount(maid_name,
                                          data,
                                          (*accounting_info_it).current_file);
               });
}

void MaidAccountHandler::DeleteDataElement(const MaidName& maid_name,
                                           const protobuf::PutData& data) {
  std::vector<MaidAcountingFileInfo>::iterator accounting_info_it;
  {
    std::lock_guard<std::mutex> guard(local_vectors_mutex_);
    FindAccountingEntry(maid_name, accounting_info_it);
    DeleteDataEntryFromFifo(maid_name, data);
  }

  active_.Send([=] () {
                 DoDeleteDataElement(maid_name,
                                     data,
                                     (*accounting_info_it).current_file);
               });
}

void MaidAccountHandler::DoDeleteDataElement(const MaidName& maid_name,
                                             const protobuf::PutData& data,
                                             int current_file) {
  NonEmptyString current_content;
  protobuf::ArchivedData archived_data;
  boost::filesystem::path filepath;
  for (int count(current_file); count != -1; --count) {
    ReadAndParseArchivedDataFile(maid_name, filepath, current_content, archived_data, current_file);
    for (int n(0); n != archived_data.put_data_size(); ++n) {
      if (ArchivedElementEntryMatchesPutData(archived_data, data, n)) {
        int32_t reps(archived_data.put_data(n).replications());
        if (reps > 1)
          archived_data.mutable_put_data(n)->set_replications(--reps);
        else
          RemoveDateElementEntryFromArchivedData(archived_data, n);
        WriteFile(filepath, archived_data.SerializeAsString());
        return;
      }
    }
  }
}

void MaidAccountHandler::RemoveDateElementEntryFromArchivedData(
    protobuf::ArchivedData& archived_data,
    int index) {
  std::vector<protobuf::PutData> temp_data;
  for (int n(0);  n != archived_data.put_data_size(); ++n) {
    if (n == index)
      continue;
    temp_data.push_back(archived_data.put_data(n));
  }

  archived_data.clear_put_data();
  for (auto& element : temp_data)
    archived_data.add_put_data()->CopyFrom(element);
}

void MaidAccountHandler::DeleteDataEntryFromFifo(const MaidName& maid_name,
                                                 const protobuf::PutData& data) {
  int index(-1);
  MaidAccountStorageIterator it = std::find_if(maid_storage_fifo_.begin(),
                                               maid_storage_fifo_.end(),
                                               [&index, &maid_name, &data, this]
                                               (const protobuf::MaidAccountStorage& maid_storage) {
                                                 return MatchMaidStorageFifoEntry(maid_storage,
                                                                                  maid_name,
                                                                                  data,
                                                                                  std::ref(index));
                                               });
  if (it != maid_storage_fifo_.end()) {
    std::vector<protobuf::PutData> temp_data;
    for (int n(0);  n != (*it).recent_put_data_size(); ++n) {
      if (n == index)
        continue;
      temp_data.push_back((*it).recent_put_data(n));
    }

    (*it).clear_recent_put_data();
    for (auto& element : temp_data)
      (*it).add_recent_put_data()->CopyFrom(element);
  }
}

void MaidAccountHandler::DoUpdateReplicationCount(const MaidName& maid_name,
                                                  const protobuf::PutData& data,
                                                  int current_file) {
  boost::filesystem::path file_path;
  protobuf::ArchivedData archive_data;
  if (!IterateArchivedElements(maid_name, data, file_path, archive_data, current_file)) {
    LOG(kError) << "Failed to perform the update of replication: " << Base64Substr(maid_name.data);
    throw std::exception();
  }
}

void MaidAccountHandler::FindAccountingEntry(const MaidName& maid_name,
                                             std::vector<MaidAcountingFileInfo>::iterator& it) {
  it = std::find_if(accounting_file_info_.begin(),
                    accounting_file_info_.end(),
                    [&maid_name] (const MaidAcountingFileInfo& file_info) {
                      return file_info.maid_name == maid_name;
                    });
  if (it == accounting_file_info_.end()) {
    LOG(kError) << "We seem to think we have the account but no record of files: "
                << Base64Substr(maid_name.data);
    throw std::exception();
  }
}

void MaidAccountHandler::FindFifoEntryAndIncrement(const MaidName& maid_name,
                                                   const protobuf::PutData& data) {
  int index(-1);
  MaidAccountStorageIterator it = std::find_if(maid_storage_fifo_.begin(),
                                               maid_storage_fifo_.end(),
                                               [&index, &maid_name, &data, this]
                                               (const protobuf::MaidAccountStorage& maid_storage) {
                                                 return MatchMaidStorageFifoEntry(maid_storage,
                                                                                  maid_name,
                                                                                  data,
                                                                                  std::ref(index));
                                               });
  if (it != maid_storage_fifo_.end() && index != -1) {
    (*it).mutable_recent_put_data(index)->set_replications(data.replications());
  } else {
    protobuf::MaidAccountStorage maid_account_storage;
    maid_account_storage.set_maid_name(maid_name.data.string());
    maid_account_storage.add_recent_put_data()->CopyFrom(data);
    maid_account_storage.mutable_recent_put_data(0)->set_replications(1);
    maid_storage_fifo_.insert(maid_storage_fifo_.begin(), maid_account_storage);
    if (maid_storage_fifo_.size() == kMaxElementSize)
      maid_storage_fifo_.pop_back();
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

void MaidAccountHandler::AddEntryInFileAndFifo(const MaidName& maid_name,
                                               const protobuf::PutData& data) {
  std::vector<MaidAcountingFileInfo>::iterator accounting_info_it;
  FindAccountingEntry(maid_name, accounting_info_it);
  ActOnAccountFiles(maid_name, data, (*accounting_info_it).current_file);
  protobuf::MaidAccountStorage maid_account_storage;
  maid_account_storage.set_maid_name(maid_name.data.string());
  maid_account_storage.add_recent_put_data()->CopyFrom(data);
  maid_storage_fifo_.insert(maid_storage_fifo_.begin(), maid_account_storage);
}

void MaidAccountHandler::ActOnAccountFiles(const MaidName& maid_name,
                                           const protobuf::PutData& data,
                                           int current_file) {
  // Find the entry to the account. Must check all files.
  boost::filesystem::path filepath;
  protobuf::ArchivedData archived_data;
  if (IterateArchivedElements(maid_name, data, filepath, archived_data, current_file))
    return;

  // Not found. Add in the newest file
  NonEmptyString current_content;
  ReadAndParseArchivedDataFile(maid_name, filepath, current_content, archived_data, current_file);
  if (archived_data.put_data_size() == static_cast<int>(kMaxElementSize)) {
    // Need a new file
    filepath = GetMaidAccountFileName(maid_name, maid_accounts_path_, current_file + 1);
    IncrementCurrentFileCounters(maid_name, false);
    archived_data.Clear();
  } else {
    IncrementCurrentFileCounters(maid_name, true);
  }
  archived_data.add_put_data()->CopyFrom(data);
  WriteFile(filepath, archived_data.SerializeAsString());
}

bool MaidAccountHandler::IterateArchivedElements(const MaidName& maid_name,
                                                 const protobuf::PutData& data,
                                                 boost::filesystem::path& filepath,
                                                 protobuf::ArchivedData& archived_data,
                                                 int current_file) {
  NonEmptyString current_content;
  for (int count(current_file); count != -1; --count) {
    ReadAndParseArchivedDataFile(maid_name, filepath, current_content, archived_data, current_file);
    for (int n(0); n != archived_data.put_data_size(); ++n) {
      if (AnalyseAndModifyArchivedElement(data, filepath, archived_data, n))
        return true;
    }
  }
  return false;
}

bool MaidAccountHandler::AnalyseAndModifyArchivedElement(const protobuf::PutData& data,
                                                         const boost::filesystem::path& filepath,
                                                         protobuf::ArchivedData& archived_data,
                                                         int n) {
  if (ArchivedElementEntryMatchesPutData(archived_data, data, n)) {
    archived_data.mutable_put_data(n)->set_replications(data.replications());
    if (!WriteFile(filepath, archived_data.SerializeAsString())) {
      LOG(kError) << "Failed to write file after increasing replication: " << filepath;
      throw std::exception();
    } else {
      return true;
    }
  }

  return false;
}

void MaidAccountHandler::ReadAndParseArchivedDataFile(const MaidName& maid_name,
                                                      boost::filesystem::path& filepath,
                                                      NonEmptyString& current_content,
                                                      protobuf::ArchivedData& archived_data,
                                                      int current_file) {
  filepath = GetMaidAccountFileName(maid_name, maid_accounts_path_, current_file);
  current_content = ReadFile(filepath);
  if (!archived_data.ParseFromString(current_content.string())) {
    LOG(kError) << "Failed to parse. Data could be corrupted: " << filepath;
    throw std::exception();
  }
}

void MaidAccountHandler::IncrementCurrentFileCounters(const MaidName& maid_name,
                                                      bool just_element_count) {
  std::lock_guard<std::mutex> guard(local_vectors_mutex_);
  auto it(std::find_if(accounting_file_info_.begin(),
                       accounting_file_info_.end(),
                       [&maid_name] (const MaidAcountingFileInfo& file_info) {
                         return file_info.maid_name == maid_name;
                       }));
  if (it == accounting_file_info_.end()) {
    LOG(kError) << "This should never happen. The entry should exist.";
    throw std::exception();
  }

  if (just_element_count) {
    ++(*it).element_count;
  } else {
    (*it).element_count = 0;
    ++(*it).current_file;
  }
}

}  // namespace vault

}  // namespace maidsafe
