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

#ifndef MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HANDLER_H_
#define MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HANDLER_H_

#include <future>
#include <string>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/active.h"

#include "maidsafe/vault/maid_account_holder/maid_account_pb.h"
#include "maidsafe/vault/types.h"

namespace maidsafe {

namespace vault {

class MaidAccountHandler {
 public:
  explicit MaidAccountHandler(const boost::filesystem::path& vault_root_dir);

  // Data operations
  void AddDataElement(const MaidName& maid_name, const protobuf::PutData& data);
  void UpdateReplicationCount(const MaidName& maid_name, const protobuf::PutData& data);
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
  struct MaidAcountingFileInfo {
    MaidAcountingFileInfo();
    MaidAcountingFileInfo(const MaidName& maid_name_in, int element_count_in, int current_file_in);
    MaidName maid_name;
    int element_count, current_file;
  };

  const boost::filesystem::path maid_accounts_path_;
  std::vector<protobuf::MaidPmidsInfo> maid_pmid_info_;
  std::vector<protobuf::MaidAccountStorage> maid_storage_fifo_;
  std::vector<MaidAcountingFileInfo> accounting_file_info_;
  Active active_;
  mutable std::mutex local_vectors_mutex_;

  void FindAccountingEntry(const MaidName& maid_name,
                           std::vector<MaidAcountingFileInfo>::iterator& it);
  void FindFifoEntryAndIncrement(const MaidName& maid_name, const protobuf::PutData& data);
  void AddEntryInFileAndFifo(const MaidName& maid_name, const protobuf::PutData& data);
  void ActOnAccountFiles(const MaidName& maid_name,
                         const protobuf::PutData& data,
                         int current_file);
  bool MatchMaidStorageFifoEntry(const protobuf::MaidAccountStorage& maid_storage,
                                 const MaidName& maid_name,
                                 const protobuf::PutData& data,
                                 int &index);
  void ReadAndParseArchivedDataFile(const MaidName& maid_name,
                                    boost::filesystem::path& filepath,
                                    NonEmptyString& current_content,
                                    protobuf::ArchivedData& archived_data,
                                    int current_file);
  bool AnalyseAndModifyArchivedElement(const protobuf::PutData& data,
                                       const boost::filesystem::path& filepath,
                                       protobuf::ArchivedData& archived_data,
                                       int n);
  void IncrementCurrentFileCounters(const MaidName& maid_name, bool just_element_count);
  bool IterateArchivedElements(const MaidName& maid_name,
                               const protobuf::PutData& data,
                               boost::filesystem::path& filepath,
                               protobuf::ArchivedData& archived_data,
                               int current_file);
  void DoUpdateReplicationCount(const MaidName& maid_name,
                                const protobuf::PutData& data,
                                int current_file);
  void DeleteDataEntryFromFifo(const MaidName& maid_name, const protobuf::PutData& data);
  void RemoveDateElementEntryFromArchivedData(protobuf::ArchivedData& archived_data,
                                              int index);
  void DoDeleteDataElement(const MaidName& maid_name,
                           const protobuf::PutData& data,
                           int current_file);
  void FindMaidInfo(const std::string& maid_name,
                    std::vector<protobuf::MaidPmidsInfo>::iterator& it);
  void FindAndUpdateTotalPutData(const MaidName& maid_name, int64_t data_increase);
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HANDLER_H_
