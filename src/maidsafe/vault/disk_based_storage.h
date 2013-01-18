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
#include <string>
#include <vector>
#include <utility>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/active.h"
#include "maidsafe/common/types.h"

namespace maidsafe {

namespace vault {

namespace protobuf { class DiskStoredElement; }

class DiskBasedStorage {
 public:
  typedef std::vector<boost::filesystem::path> PathVector;
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
  void WriteFile(const boost::filesystem::path& path, const NonEmptyString& content);

 private:
  const boost::filesystem::path kRoot_;
  mutable Active active_;
  typedef std::pair<uint32_t, std::string> FileData;
  std::vector<FileData> file_data_;

  template<typename Data>
  void DoStore(const typename Data::name_type& name,
               int32_t version,
               const std::string& serialised_value);
  template<typename Data>
  void DoDelete(const typename Data::name_type& name, int32_t version);
  template<typename Data>
  void DoModify(const typename Data::name_type& name,
                int32_t version,
                const std::function<void(std::string&)>& functor,
                const std::string& serialised_value);
  void DoGetFileNames(std::shared_ptr<std::promise<PathVector> > promise) const;
  void DoWriteFile(const boost::filesystem::path& path, const NonEmptyString& content);

  void AddToLatestFile(const protobuf::DiskStoredElement& element);
  void SearchAndDeleteEntry(const protobuf::DiskStoredElement& element);
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/disk_based_storage-inl.h"

#endif  // MAIDSAFE_VAULT_DISK_BASED_STORAGE_H_
