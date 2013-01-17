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

#include "maidsafe/vault/disk_based_storage_messages_pb.h"

namespace maidsafe {

namespace vault {

class DiskBasedStorage {
  // Initialise with root.leaf() == MAID name / PMID name, etc.
  explicit DiskBasedStorage(const boost::filesystem::path& root);

  // Element handling
  template<typename Data>
  void Store(typename const Data::name_type& name,
             int32_t version,
             const std::string& serialised_value);
  template<typename Data>
  void Delete(typename const Data::name_type& name, int32_t version);
  template<typename Data>
  void Modify(typename const Data::name_type& name,
              int32_t version,
              const std::function<void(std::string&)>& functor,
              const std::string& serialised value);

  // Synchronisation helpers
  uint32_t GetFileCount() const;
  // File names are index no. + hash of contents
  std::vector<boost::filesystem::path> GetFileNames() const;
  void WriteFile(const boost::filesystem::path& path, const NonEmptyString& content);
  std::future<NonEmptyString> GetFile(const boost::filesystem::path& path) const;

 private:
  const boost::filesystem::path kRoot_;
  mutable Active active_;
  typedef std::pair<uint32_t, std::string> FileData;
  std::vector<FileData> file_data_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/disk_based_storage-inl.h"

#endif  // MAIDSAFE_VAULT_DISK_BASED_STORAGE_H_
