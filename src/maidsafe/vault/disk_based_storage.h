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

#ifndef MAIDSAFE_VAULT_DISK_BASED_STORAGE_H_
#define MAIDSAFE_VAULT_DISK_BASED_STORAGE_H_

#include <cstdint>
#include <functional>
#include <future>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "maidsafe/common/bounded_string.h"
#include "maidsafe/common/crypto.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/nfs/data_message.h"

#include "maidsafe/vault/disk_based_storage.pb.h"


namespace maidsafe {

namespace vault {

namespace test { class DiskStorageTest; }

class DiskBasedStorage {
 public:
  struct RecentOperation {
    RecentOperation(const DataNameVariant& data_name_variant_in,
                    int32_t size_in,
                    nfs::DataMessage::Action action_in);
    RecentOperation(const RecentOperation& other);
    RecentOperation& operator=(const RecentOperation& other);
    RecentOperation(RecentOperation&& other);
    RecentOperation& operator=(RecentOperation&& other);
    DataNameVariant data_name_variant;
    // Could represent cost in context of MAH, or data_size in context of PAH.
    int32_t size;
    nfs::DataMessage::Action action;
    int sync_attempts;
  };

  explicit DiskBasedStorage(const boost::filesystem::path& root);
  DiskBasedStorage(DiskBasedStorage&&);
  DiskBasedStorage& operator=(DiskBasedStorage&&);

  template<typename Data>
  size_t GetElementCount(const typename Data::name_type& name) const;

  // Synchronisation helpers
  void ApplyRecentOperations(const std::vector<RecentOperation>& recent_ops);
  void ApplyAccountTransfer(const boost::filesystem::path& transferred_files_dir);
  std::vector<boost::filesystem::path> GetFilenames() const;
  NonEmptyString GetFile(const boost::filesystem::path& filename) const;

  friend class test::DiskStorageTest;

 private:
  DiskBasedStorage(const DiskBasedStorage&);
  DiskBasedStorage& operator=(const DiskBasedStorage&);

  struct FileDetails {
    enum { kSubstrSize = 5 };
    typedef maidsafe::detail::BoundedString<kSubstrSize, kSubstrSize> ElementNameSubstr;
    FileDetails(const protobuf::DiskStoredFile& file, const crypto::SHA1Hash& hash_in);
    FileDetails(const FileDetails& other);
    FileDetails& operator=(const FileDetails& other);
    FileDetails(FileDetails&& other);
    FileDetails& operator=(FileDetails&& other);
    ElementNameSubstr min_element;
    ElementNameSubstr max_element;
    crypto::SHA1Hash hash;
  };
  typedef std::multimap<DataNameVariant, int32_t> Elements;
  typedef Elements::value_type Element;
  typedef std::map<int, FileDetails> FileGroup;

  boost::filesystem::path GetFileName(const FileGroup::value_type& file_id) const;
  std::pair<protobuf::DiskStoredFile, FileDetails> ParseFile(
      const FileGroup::value_type& file_id,
      bool verify) const;
  std::pair<protobuf::DiskStoredFile, FileDetails> ParseFile(
      const boost::filesystem::path& file_path,
      bool verify) const;
  void VerifyFile(std::pair<protobuf::DiskStoredFile, FileDetails>& file_and_details) const;
  void VerifyFileGroup() const;

  void SetCurrentOps(std::vector<RecentOperation> recent_ops);
  FileGroup::iterator GetFileIdsLowerBound(
      const FileDetails::ElementNameSubstr& element_name_substr);
  FileGroup::iterator GetReorganiseStartPoint();
  void SaveFile(FileGroup::iterator file_ids_itr);
  protobuf::DiskStoredFile MoveCurrentPutsToFile();
  FileGroup GetFilesToTransfer(const boost::filesystem::path& transferred_files_dir) const;
  void TransferFile(const FileGroup::value_type& transfer_id,
                    const boost::filesystem::path& transferred_files_dir,
                    std::vector<boost::filesystem::path>& files_moved,
                    std::vector<boost::filesystem::path>& files_to_be_removed);

  boost::filesystem::path root_;
  Elements current_puts_, current_deletes_;
  FileGroup file_ids_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/disk_based_storage-inl.h"

#endif  // MAIDSAFE_VAULT_DISK_BASED_STORAGE_H_
