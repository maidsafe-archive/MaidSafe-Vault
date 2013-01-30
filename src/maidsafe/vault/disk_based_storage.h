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

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/active.h"
#include "maidsafe/common/crypto.h"
#include "maidsafe/common/types.h"

#include "maidsafe/vault/disk_based_storage_messages_pb.h"


namespace maidsafe {

namespace vault {

namespace test {  template<class T> class DiskStorageTest; }

class DiskBasedStorage {
 public:
  typedef std::vector<boost::filesystem::path> PathVector;
  typedef std::map<std::string, std::string> OrderingMap;
  // Initialise with root.leaf() == MAID name / PMID name, etc.
  explicit DiskBasedStorage(const boost::filesystem::path& root);

  // Element handling
  template<typename Data>
  std::future<void> Store(const typename Data::name_type& name,
                          const std::string& serialised_value);
  template<typename Data>
  std::future<void> Delete(const typename Data::name_type& name);
  template<typename Data>
  std::future<void> Modify(const typename Data::name_type& name,
                           const std::function<void(std::string&)>& functor,
                           const std::string& serialised_value);

  // Synchronisation helpers
  std::future<uint32_t> GetFileCount() const;
  std::future<PathVector> GetFileNames() const;  // File names are index no. + hash of contents
  std::future<NonEmptyString> GetFile(const boost::filesystem::path& path) const;
  void PutFile(const boost::filesystem::path& path, const NonEmptyString& content);

  template<class T> friend class test::DiskStorageTest;
 private:
  class Changer;
  template<typename Data> struct StoredElement;

  DiskBasedStorage(const DiskBasedStorage&);
  DiskBasedStorage& operator=(const DiskBasedStorage&);
  DiskBasedStorage(DiskBasedStorage&&);
  DiskBasedStorage& operator=(DiskBasedStorage&&);

  void TraverseAndVerifyFiles(const boost::filesystem::path& root);
  uint32_t VerifyFileHashAndCountElements(const crypto::SHA512Hash& hash, size_t file_number);
  void AddToFileData(const crypto::SHA512Hash& hash, size_t file_number, uint32_t element_count);
  void DoPutFile(const boost::filesystem::path& path,
                 const NonEmptyString& content,
                 size_t file_number,
                 const crypto::SHA512Hash& hash);

  template<typename Data>
  void AddToLatestFile(const StoredElement<Data>& element);
  template<typename Data>
  void SearchForAndDeleteEntry(const StoredElement<Data>& element);
  template<typename Data>
  void SearchForAndModifyEntry(const StoredElement<Data>& element,
                               const std::function<void(std::string&)>& functor);
  template<typename Data>
  void SearchForEntryAndExecuteOperation(const StoredElement<Data>& element,
                                         Changer& changer,
                                         bool reorder);
  void ReadAndParseFile(const crypto::SHA512Hash& hash,
                        size_t file_index,
                        protobuf::DiskStoredFile& disk_file,
                        boost::filesystem::path& file_path,
                        NonEmptyString& file_content) const;
  void UpdateFileAfterModification(std::vector<crypto::SHA512Hash>::reverse_iterator& it,
                                   size_t file_index,
                                   const protobuf::DiskStoredFile& disk_file,
                                   const boost::filesystem::path& file_path,
                                   bool reorder);
  void MergeFilesAfterDelete();
  bool CheckSpecialMergeCases();
  void HandleNextFile(const boost::filesystem::path& current_path,
                      const protobuf::DiskStoredFile& current_file_disk,
                      OrderingMap& ordering,
                      size_t& current_counter,
                      size_t& new_counter);

  template<typename Data>
  void AddDiskElementToFileDisk(const DiskBasedStorage::StoredElement<Data>& element,
                                protobuf::DiskStoredFile& disk_file);

  // Helper functions
  static boost::filesystem::path GetFilePath(const boost::filesystem::path& base_path,
                                             const crypto::SHA512Hash& hash,
                                             size_t file_number);
  template<typename Data>
  bool MatchingDiskElements(const protobuf::DiskStoredElement& lhs,
                            const DiskBasedStorage::StoredElement<Data>& element) const;

  const boost::filesystem::path kRoot_;
  mutable Active active_;
  std::vector<crypto::SHA512Hash> file_hashes_;
  const crypto::SHA512Hash kEmptyFileHash_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/disk_based_storage-inl.h"

#endif  // MAIDSAFE_VAULT_DISK_BASED_STORAGE_H_
