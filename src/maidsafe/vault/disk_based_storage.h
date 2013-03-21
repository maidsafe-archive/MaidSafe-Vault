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
  };

  explicit DiskBasedStorage(const boost::filesystem::path& root);

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
  DiskBasedStorage(DiskBasedStorage&&);
  DiskBasedStorage& operator=(DiskBasedStorage&&);

  struct FileDetails {
    typedef TaggedValue<maidsafe::detail::BoundedString<5, 5>, struct MinTag> MinElement;
    typedef TaggedValue<maidsafe::detail::BoundedString<5, 5>, struct MaxTag> MaxElement;
    FileDetails();
    FileDetails(const FileDetails& other);
    FileDetails& operator=(const FileDetails& other);
    FileDetails(FileDetails&& other);
    FileDetails& operator=(FileDetails&& other);
    MinElement min_element;
    MaxElement max_element;
    crypto::SHA1Hash hash;
  };
  typedef std::multimap<DataNameVariant, int32_t> Elements;
  typedef Elements::value_type Element;
  typedef std::map<int, FileDetails> FileGroup;

  boost::filesystem::path GetFileName(const FileGroup::value_type& file_id) const;
  protobuf::DiskStoredFile ParseFile(const FileGroup::value_type& file_id) const;
  protobuf::DiskStoredFile ParseFile(const boost::filesystem::path& file_path) const;

  void SetCurrentOps(const std::vector<RecentOperation>& recent_ops);
  FileGroup::iterator GetReorganiseStartPoint();










  template<typename Data>
  void AddToLatestFile(const typename Data::name_type& name, int32_t value);
  template<typename Data>
  void AddElement(const typename Data::name_type& name,
                  int32_t value,
                  const FileIdentity& file_id,
                  protobuf::DiskStoredFile& file);

  template<typename Data>
  void FindAndDeleteEntry(const typename Data::name_type& name, int32_t& value);
  template<typename Data>
  int GetEntryIndex(const typename Data::name_type& name,
                    const protobuf::DiskStoredFile& file) const;
  void DeleteEntry(int index, protobuf::DiskStoredFile& file) const;

  void SaveChangedFile(const FileIdentity& file_id, const protobuf::DiskStoredFile& file);

  void ReadIntoMemory(FileIdentities::iterator &read_itr,
                      std::vector<protobuf::DiskStoredElement>& elements);
  void WriteToDisk(FileIdentities::iterator &write_itr,
                   std::vector<protobuf::DiskStoredElement>& elements);
  void PruneFilesToEnd(const FileIdentities::iterator& first_itr);

  void DoPutFile(const boost::filesystem::path& filename,
                 const NonEmptyString& content,
                 const FileIdentity& file_id);

  const boost::filesystem::path kRoot_;
  Elements current_puts_, current_deletes_;
  FileIdentities file_ids_;
};

inline bool operator<(const DiskBasedStorage::FileIdentity& lhs,
                      const DiskBasedStorage::FileIdentity& rhs) {
  return lhs.min < rhs.min;
}

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/disk_based_storage-inl.h"

#endif  // MAIDSAFE_VAULT_DISK_BASED_STORAGE_H_
