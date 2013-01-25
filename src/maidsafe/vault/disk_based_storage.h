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
#include "maidsafe/common/types.h"

#include "maidsafe/vault/disk_based_storage_messages_pb.h"


namespace maidsafe {

namespace vault {

class DiskBasedStorage {
 public:
  typedef std::vector<boost::filesystem::path> PathVector;
  typedef std::map<std::string, std::pair<int32_t, std::string>> OrderingMap;
  // Initialise with root.leaf() == MAID name / PMID name, etc.
  explicit DiskBasedStorage(const boost::filesystem::path& root);

  // Element handling
  template<typename Data>
  void Store(const typename Data::name_type& name,
             int32_t version,
             const std::string& serialised_value);
  template<typename Data>
  void Delete(const typename Data::name_type& name, int32_t version);
  template<typename Data>
  void Modify(const typename Data::name_type& name,
              int32_t version,
              const std::function<void(std::string&)>& functor,
              const std::string& serialised_value);

  // Synchronisation helpers
  std::future<uint32_t> GetFileCount() const;
  std::future<PathVector> GetFileNames() const;  // File names are index no. + hash of contents
  std::future<NonEmptyString> GetFile(const boost::filesystem::path& path) const;
  void PutFile(const boost::filesystem::path& path, const NonEmptyString& content);

 private:
  class Changer;

  DiskBasedStorage(const DiskBasedStorage&);
  DiskBasedStorage& operator=(const DiskBasedStorage&);
  DiskBasedStorage(DiskBasedStorage&&);
  DiskBasedStorage& operator=(DiskBasedStorage&&);

  void TraverseAndVerifyFiles(const boost::filesystem::path& root);
  uint32_t VerifyFileHashAndCountElements(const std::string& hash, size_t file_number);
  void AddToFileData(const std::string& hash, size_t file_number, uint32_t element_count);
  void DoPutFile(const boost::filesystem::path& path, const NonEmptyString& content);

  void AddToLatestFile(const protobuf::DiskStoredElement& element);
  void SearchForAndDeleteEntry(const protobuf::DiskStoredElement& element);
  void SearchForAndModifyEntry(const protobuf::DiskStoredElement& element,
                               const std::function<void(std::string&)>& functor);
  void SearchForEntryAndExecuteOperation(const protobuf::DiskStoredElement& element,
                                         Changer& changer,
                                         bool reorder);
  void ReadAndParseFile(const std::string& hash,
                        size_t file_index,
                        protobuf::DiskStoredFile& disk_file,
                        boost::filesystem::path& file_path,
                        NonEmptyString& file_content) const;
  void UpdateFileAfterModification(std::vector<std::string>::reverse_iterator& it,
                                   size_t file_index,
                                   protobuf::DiskStoredFile& disk_file,
                                   boost::filesystem::path& file_path,
                                   bool reorder);
  void MergeFilesAfterAlteration(size_t current_index);
  void AddToDiskFile(const boost::filesystem::path& previous_path,
                     protobuf::DiskStoredFile& previous_file_disk,
                     OrderingMap::reverse_iterator& r_it,
                     size_t file_index,
                     size_t begin,
                     size_t end);

  const boost::filesystem::path kRoot_;
  mutable Active active_;
  std::vector<std::string> file_hashes_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/disk_based_storage-inl.h"

#endif  // MAIDSAFE_VAULT_DISK_BASED_STORAGE_H_
