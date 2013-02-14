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

#ifndef MAIDSAFE_VAULT_MAID_ACCOUNT_HANDLER_H_
#define MAIDSAFE_VAULT_MAID_ACCOUNT_HANDLER_H_

#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/types.h"
#include "maidsafe/nfs/pmid_registration.h"

#include "maidsafe/vault/maid_account.h"
#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

namespace test {

class MaidAccountHandlerTest;

}  // namespace test

class MaidAccountHandler {
 public:
  explicit MaidAccountHandler(const boost::filesystem::path& vault_root_dir);
  // Account operations
  bool AddAccount(std::unique_ptr<MaidAccount>&& maid_account);
  bool DeleteAccount(const MaidName& account_name);

  void RegisterPmid(const MaidName& account_name, const nfs::PmidRegistration& pmid_registration);
  void UnregisterPmid(const MaidName& account_name, const PmidName& pmid_name);
  void UpdatePmidTotals(const MaidName& account_name, const PmidTotals& pmid_totals);

  // Sync operations
  std::vector<MaidName> GetAccountNames() const;
  MaidAccount::serialised_type GetSerialisedAccount(const MaidName& account_name) const;
  std::vector<boost::filesystem::path> GetArchiveFileNames(const MaidName& account_name) const;
  NonEmptyString GetArchiveFile(const MaidName& account_name,
                                const boost::filesystem::path& filename) const;
  void PutArchiveFile(const MaidName& account_name,
                      const boost::filesystem::path& filename,
                      const NonEmptyString& content);

  // Data operations
  template<typename Data>
  void PutData(const MaidName& account_name,
               const typename Data::name_type& data_name,
               int32_t cost);
  template<typename Data>
  void DeleteData(const MaidName& account_name, const typename Data::name_type& data_name);
  template<typename Data>
  void UpdateReplicationCount(const MaidName& account_name,
                              const typename Data::name_type& data_name,
                              int32_t new_replication_count);

  friend class test::MaidAccountHandlerTest;

 private:
  MaidAccountHandler(const MaidAccountHandler&);
  MaidAccountHandler& operator=(const MaidAccountHandler&);
  MaidAccountHandler(MaidAccountHandler&&);
  MaidAccountHandler& operator=(MaidAccountHandler&&);

//  void FindAccountingEntry(const MaidName& maid_name,
//                           std::vector<MaidAcountingFileInfo>::iterator& it);
//  void FindAccountingEntry(const MaidName& maid_name,
//                           std::vector<MaidAcountingFileInfo>::const_iterator& it) const;
//  void FindFifoEntryAndIncrement(const MaidName& maid_name, const protobuf::PutData& data);
//  void AddEntryInFileAndFifo(const MaidName& maid_name, const protobuf::PutData& data);
//  void ActOnAccountFiles(const MaidName& maid_name,
//                         const protobuf::PutData& data,
//                         int current_file);
//  bool MatchMaidStorageFifoEntry(const protobuf::MaidAccountStorage& maid_storage,
//                                 const MaidName& maid_name,
//                                 const protobuf::PutData& data,
//                                 int &index);
//  void ReadAndParseArchivedDataFile(const MaidName& maid_name,
//                                    boost::filesystem::path& filepath,
//                                    NonEmptyString& current_content,
//                                    protobuf::ArchivedData& archived_data,
//                                    int current_file) const;
//  bool AnalyseAndModifyArchivedElement(const protobuf::PutData& data,
//                                       const boost::filesystem::path& filepath,
//                                       protobuf::ArchivedData& archived_data,
//                                       int n);
//  void IncrementCurrentFileCounters(const MaidName& maid_name, bool just_element_count);
//  bool IterateArchivedElements(const MaidName& maid_name,
//                               const protobuf::PutData& data,
//                               boost::filesystem::path& filepath,
//                               protobuf::ArchivedData& archived_data,
//                               int current_file);
//  void DoUpdateReplicationCount(const MaidName& maid_name,
//                                const protobuf::PutData& data,
//                                int current_file);
//  void DeleteDataEntryFromFifo(const MaidName& maid_name, const protobuf::PutData& data);
//  void RemoveDateElementEntryFromArchivedData(protobuf::ArchivedData& archived_data,
//                                              int index);
//  void DoDeleteDataElement(const MaidName& maid_name,
//                           const protobuf::PutData& data,
//                           int current_file);
//  void FindMaidInfo(const std::string& maid_name,
//                    std::vector<protobuf::MaidPmidsInfo>::iterator& it);
//  void FindAndUpdateTotalPutData(const MaidName& maid_name, int64_t data_increase);
//  void ReadFileContentsIntoString(const MaidName& maid_name,
//                                  size_t index,
//                                  std::shared_ptr<std::promise<std::string> > promise) const;

  const boost::filesystem::path kMaidAccountsRoot_;
  mutable std::mutex mutex_;
  std::vector<std::unique_ptr<MaidAccount>> maid_accounts_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/maid_account_handler-inl.h"

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_HANDLER_H_
