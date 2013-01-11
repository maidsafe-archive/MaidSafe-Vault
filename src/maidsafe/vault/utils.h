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

#ifndef MAIDSAFE_VAULT_UTILS_H_
#define MAIDSAFE_VAULT_UTILS_H_

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include "unordered_map"
#include "boost/filesystem/path.hpp"

#include "maidsafe/common/rsa.h"
#include "maidsafe/common/active.h"


namespace maidsafe {

namespace vault {

// will confirm signature matches src ID private key
// signed the type (three enums) and payload. Will do a Get from MM
bool checkMessageSignature(nfs::Message& message);

template <typename T>
class MaidAccoundHolderStorage {
 public:
  MaidAccoundHolderStorage();
  bool Put(Identity name, int32_t size);
  bool Delete(Identity name);
 protected:
  ~MaidAccoundHolderStorage();
};

template <typename T>
class PmidAccoundHolderStorage {
 public:
  PmidAccoundHolderStorage();
  bool Put(Identity name, int32_t size);
  bool Delete(Identity name);
 protected:
  ~PmidAccoundHolderStorage();
};

// default file size 1Mb
// Will create a binary split file
// this can probably be ordered in oldest first (by file as a clump)
// can be sorted in memory for efficiency but only read the file we need
// so in normal operation only load most recent copy
// only for delete back in time will we need to open older files
// multiple copies will have multiple entries (maybe some in newer files)
template <class StoragePolicy>  // must provide Put()->bool / Delete->bool
                                // Serialise() and Parse()
class FileSplitter : public StoragePolicy {
 public:
  explicit FileSplitter(boost::filesystem::path fullname);  // read all files
  ~FileSplitter();  // store all files

 private:
};

template <class T>
class DiskBasedStorage {
  explicit DiskBasedStorage(boost::filesystem::path name);
  bool Put(Identity name);
  T Get(Identity name);
  bool Delete(Identity name);
  std::vector<Identity> ReadAll();
 private:
  boost::filesystem::path name_;
  FileSplitter<T> file_splitter_;
  Active active_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_UTILS_H_
