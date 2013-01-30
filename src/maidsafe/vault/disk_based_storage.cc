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

#include "maidsafe/vault/disk_based_storage.h"

#include <algorithm>
#include <limits>
#include <memory>

#include "boost/filesystem/operations.hpp"

#include "maidsafe/common/utils.h"

#include "maidsafe/vault/parameters.h"
#include "maidsafe/vault/types.h"

namespace fs = boost::filesystem;

namespace maidsafe {

namespace vault {

namespace {

const crypto::SHA512Hash kEmptyFileHash("filehashfilehashfilehashfilehash"
                                        "filehashfilehashfilehashfilehash");

typedef std::promise<NonEmptyString> NonEmptyStringPromise;
typedef std::promise<uint32_t> Uint32tPromise;
typedef std::promise<DiskBasedStorage::PathVector> VectorPathPromise;

void AddDataToElement(const DiskBasedStorage::OrderingMap::iterator& it,
                      protobuf::DiskStoredElement*& disk_element) {
  disk_element->set_data_name((*it).first);
  disk_element->set_serialised_value((*it).second);
}

void ExtractElementsFromFilename(const std::string& filename,
                                 crypto::SHA512Hash& hash,
                                 size_t& file_number) {
  auto it(std::find(filename.begin(), filename.end(), '.'));
  if (it == filename.end()) {
    LOG(kError) << "No dot in the file name.";
    ThrowError(VaultErrors::failed_elements_extraction);
  }
  file_number = static_cast<size_t>(std::stoi(std::string(filename.begin(), it)));
  hash = crypto::SHA512Hash(DecodeFromBase32(std::string(it + 1, filename.end())));
}

boost::filesystem::path GetFileName(const crypto::SHA512Hash& hash, size_t file_number) {
  return boost::filesystem::path(std::to_string(file_number) + "." + EncodeToBase32(hash));
}

}  // namespace

class DiskBasedStorage::Changer {
 public:
  Changer() : functor_(nullptr) {}
  explicit Changer(const std::function<void(std::string&)> functor) : functor_(functor) {}

  void Execute(protobuf::DiskStoredFile& disk_file, int index) {
    if (functor_)
      ModifyElement(disk_file, index);
    else
      RemoveElement(disk_file, index);
  }

 private:
  std::function<void(std::string&)> functor_;

  void RemoveElement(protobuf::DiskStoredFile& disk_file, int index) {
    protobuf::DiskStoredFile temp;
    for (int n(0); n != disk_file.disk_element_size(); ++n) {
      if (n != index)
        temp.add_disk_element()->CopyFrom(disk_file.disk_element(n));
    }

    disk_file = temp;
  }

  void ModifyElement(protobuf::DiskStoredFile& disk_file, int index) {
    std::string element_to_modify(disk_file.disk_element(index).SerializeAsString());
    functor_(element_to_modify);
    protobuf::DiskStoredElement modified_element;
    assert(modified_element.ParseFromString(element_to_modify) &&
           "Returned element doesn't parse.");
    disk_file.mutable_disk_element(index)->CopyFrom(modified_element);
  }
};

DiskBasedStorage::DiskBasedStorage(const fs::path& root)
    : kRoot_(root),
      active_(),
      file_hashes_() {
  if (fs::exists(root)) {
    if (fs::is_directory(root))
      TraverseAndVerifyFiles(root);
    else
      ThrowError(VaultErrors::path_not_a_directory);
  } else {
    fs::create_directory(root);
  }

  if (file_hashes_.empty())
    file_hashes_.push_back(kEmptyFileHash);
}

void DiskBasedStorage::TraverseAndVerifyFiles(const fs::path& root) {
  crypto::SHA512Hash hash;
  size_t file_number(std::numeric_limits<size_t>::max());
  fs::directory_iterator root_itr(root), end_itr;
  for (; root_itr != end_itr; ++root_itr) {
    try {
      ExtractElementsFromFilename(fs::path(*root_itr).filename().string(),
                                  hash,
                                  file_number);
      uint32_t element_count(VerifyFileHashAndCountElements(hash, file_number));
      AddToFileData(hash, file_number, element_count);
    }
    catch(const std::exception& e) {
      LOG(kWarning) << "Failed to parse file " << *root_itr << ": " << e.what();
    }
  }
}

uint32_t DiskBasedStorage::VerifyFileHashAndCountElements(const crypto::SHA512Hash& hash,
                                                          size_t file_number) {
  protobuf::DiskStoredFile disk_file;
  fs::path file_path;
  NonEmptyString file_content;
  ReadAndParseFile(hash, file_number, disk_file, file_path, file_content);
  if (crypto::Hash<crypto::SHA512>(file_content) != hash) {
    LOG(kInfo) << "Contents don't hash to what file name contains.";
    ThrowError(VaultErrors::hash_failure);
  }
  return static_cast<uint32_t>(disk_file.disk_element_size());
}

void DiskBasedStorage::AddToFileData(const crypto::SHA512Hash& hash,
                                     size_t file_number,
                                     uint32_t element_count) {
  if (element_count >= file_hashes_.size()) {
    file_hashes_.resize(element_count - 1, kEmptyFileHash);
    file_hashes_.push_back(hash);
  } else if (file_hashes_.at(file_number) == hash) {
    LOG(kInfo) << "Already filled file index: " << file_number;
  } else if (file_hashes_.at(file_number) == kEmptyFileHash) {
    file_hashes_.at(file_number) = hash;
  } else {
    LOG(kWarning) << "Unexpected content in vector: " << file_number;
  }
}

std::future<uint32_t> DiskBasedStorage::GetFileCount() const {
  std::shared_ptr<Uint32tPromise> promise(std::make_shared<Uint32tPromise>());
  std::future<uint32_t> future(promise->get_future());
  active_.Send([promise, this] () {
                 promise->set_value(static_cast<uint32_t>(file_hashes_.size()));
               });
  return std::move(future);
}

std::future<DiskBasedStorage::PathVector> DiskBasedStorage::GetFileNames() const {
  std::shared_ptr<VectorPathPromise> promise(std::make_shared<VectorPathPromise>());
  std::future<DiskBasedStorage::PathVector> future(promise->get_future());
  active_.Send([promise, this] {
                   std::vector<fs::path> file_names;
                   for (size_t n(0); n < file_hashes_.size(); ++n) {
                     if (file_hashes_.at(n) != kEmptyFileHash)
                       file_names.push_back(GetFileName(file_hashes_.at(n), n));
                   }
                   promise->set_value(file_names);
               });
  return std::move(future);
}

std::future<NonEmptyString> DiskBasedStorage::GetFile(const fs::path& path) const {
  // Check path? It might not exist anymore because of operations in the queue
  // crypto::SHA512Hash hash
  // size_t file_number;
  // ExtractElementsFromFilename(path.filename().string(), hash, file_number);
  // if (file_data_.at(file_number) != hash)
  //   ThrowError(VaultErrors::hash_failure);
  std::shared_ptr<NonEmptyStringPromise> promise(std::make_shared<NonEmptyStringPromise>());
  std::future<NonEmptyString> future(promise->get_future());
  active_.Send([path, promise, this] () {
                 try {
                   promise->set_value(ReadFile(kRoot_ / path));
                 }
                 catch(...) {
                   promise->set_exception(std::current_exception());
                 }
               });
  return std::move(future);
}

void DiskBasedStorage::PutFile(const fs::path& path, const NonEmptyString& content) {
  size_t file_number;
  crypto::SHA512Hash hash;
  ExtractElementsFromFilename(path.string(), hash, file_number);
  if (crypto::Hash<crypto::SHA512>(content) != hash) {
    LOG(kError) << "Content doesn't hash.";
    ThrowError(VaultErrors::hash_failure);
  }
  active_.Send([path, content, file_number, hash, this] () {
                 DoPutFile(kRoot_ / path, content, file_number, hash);
               });
}

void DiskBasedStorage::DoPutFile(const fs::path& path,
                                 const NonEmptyString& content,
                                 size_t file_number,
                                 const crypto::SHA512Hash& hash) {
  protobuf::DiskStoredFile disk_file;
  assert(disk_file.ParseFromString(content.string()));
  if (file_number < file_hashes_.size()) {
    crypto::SHA512Hash old_hash(file_hashes_.at(file_number));
    assert(old_hash != hash && "Hash is the same as it's currently held.");
    file_hashes_.at(file_number) = hash;
    fs::remove(GetFilePath(kRoot_, old_hash, file_number));
  } else {
    while (file_number > file_hashes_.size())
      file_hashes_.push_back(kEmptyFileHash);
    file_hashes_.push_back(hash);
  }
  WriteFile(path, content.string());
}

void DiskBasedStorage::AddToLatestFile(const protobuf::DiskStoredElement& element) {
  size_t latest_file_index(file_hashes_.size() - 1);
  protobuf::DiskStoredFile disk_file;
  fs::path latest_file_path;
  NonEmptyString file_content;
  if (file_hashes_.at(latest_file_index) != kEmptyFileHash) {
    ReadAndParseFile(file_hashes_.at(latest_file_index),
                     latest_file_index,
                     disk_file,
                     latest_file_path,
                     file_content);
  }

  disk_file.add_disk_element()->CopyFrom(element);
  NonEmptyString new_content(disk_file.SerializeAsString());
  file_hashes_.at(latest_file_index) = crypto::Hash<crypto::SHA512>(new_content);
  if (disk_file.disk_element_size() == int(detail::Parameters::max_file_element_count())) {
    // Increment the file
    file_hashes_.push_back(kEmptyFileHash);
  }

  fs::path new_path(GetFilePath(kRoot_, file_hashes_.at(latest_file_index), latest_file_index));
  WriteFile(new_path, new_content.string());
  if ((!latest_file_path.empty()) && (latest_file_path != new_path))
    fs::remove(latest_file_path);
}

void DiskBasedStorage::SearchForAndDeleteEntry(const protobuf::DiskStoredElement& element) {
  Changer changer;
  SearchForEntryAndExecuteOperation(element, changer, true);
}

void DiskBasedStorage::SearchForAndModifyEntry(const protobuf::DiskStoredElement& element,
                                               const std::function<void(std::string&)>& functor) {
  Changer changer(functor);
  SearchForEntryAndExecuteOperation(element, changer, false);
}

void DiskBasedStorage::SearchForEntryAndExecuteOperation(const protobuf::DiskStoredElement& element,
                                                         Changer& changer,
                                                         bool reorder) {
  size_t file_index(file_hashes_.size() - 1);
  fs::path file_path;
  protobuf::DiskStoredFile disk_file;
  NonEmptyString file_content;
  for (auto it(file_hashes_.rbegin()); it != file_hashes_.rend(); ++it, --file_index) {
    ReadAndParseFile(*it, file_index, disk_file, file_path, file_content);
    for (int n(disk_file.disk_element_size() - 1); n != -1; --n) {
      if (MatchingDiskElements(disk_file.disk_element(n), element)) {
        changer.Execute(disk_file, n);
        UpdateFileAfterModification(it, file_index, disk_file, file_path, reorder);
        return;
      }
    }
  }
}

void DiskBasedStorage::ReadAndParseFile(const crypto::SHA512Hash& hash,
                                        size_t file_index,
                                        protobuf::DiskStoredFile& disk_file,
                                        fs::path& file_path,
                                        NonEmptyString& file_content) const {
  disk_file.Clear();
  if (hash == kEmptyFileHash) {
    LOG(kInfo) << "Empty file, no need to read.";
    return;
  }
  file_path = GetFilePath(kRoot_, hash, file_index);
  file_content = ReadFile(file_path);
  if (!disk_file.ParseFromString(file_content.string())) {
    LOG(kError) << "Failure to parse file content.";
    ThrowError(VaultErrors::parse_failure);
  }
}

void DiskBasedStorage::UpdateFileAfterModification(
    std::vector<crypto::SHA512Hash>::reverse_iterator& it,
    size_t file_index,
    const protobuf::DiskStoredFile& disk_file,
    const fs::path& file_path,
    bool reorder) {
  try {
    NonEmptyString file_content(NonEmptyString(disk_file.SerializeAsString()));
    *it = crypto::Hash<crypto::SHA512>(file_content);
    WriteFile(file_path, file_content.string());
    fs::rename(file_path, GetFilePath(kRoot_, *it, file_index));
    if (reorder &&
        disk_file.disk_element_size() < int(detail::Parameters::min_file_element_count())) {
      MergeFilesAfterDelete();
    }
  } catch(const std::exception& /*e*/) {
    *it = kEmptyFileHash;
    fs::remove(file_path);
  }
}

void DiskBasedStorage::MergeFilesAfterDelete() {
  if (CheckSpecialMergeCases())
    return;

  OrderingMap ordering;
  size_t current_counter(1), new_counter(0), file_hashes_max(file_hashes_.size());
  while (current_counter != file_hashes_max) {
    fs::path current_path(GetFilePath(kRoot_, file_hashes_.at(new_counter), new_counter));
    NonEmptyString current_content(ReadFile(current_path));
    protobuf::DiskStoredFile current_file_disk;
    current_file_disk.ParseFromString(current_content.string());
    HandleNextFile(current_path, current_file_disk, ordering, current_counter, new_counter);
  }
}

bool DiskBasedStorage::CheckSpecialMergeCases() {
  // Handle single file case
  if (file_hashes_.size() == 1U) {
    LOG(kInfo) << "Just one file. No need to merge.";
    return true;
  }

  return false;
}

void DiskBasedStorage::HandleNextFile(const boost::filesystem::path& current_path,
                                      const protobuf::DiskStoredFile& current_file_disk,
                                      OrderingMap& ordering,
                                      size_t& current_counter,
                                      size_t& new_counter) {
  for (int n(0); n != current_file_disk.disk_element_size(); ++n) {
    ordering.insert(std::make_pair(current_file_disk.disk_element(n).data_name(),
                                   current_file_disk.disk_element(n).serialised_value()));
  }

  NonEmptyString content;
  protobuf::DiskStoredFile file_disk;
  while (ordering.size() < detail::Parameters::max_file_element_count() ||
         current_counter != file_hashes_.size()) {
    // Read another file and add elements
    content = ReadFile(GetFilePath(kRoot_, file_hashes_.at(current_counter), current_counter));
    ++current_counter;
    file_disk.ParseFromString(content.string());
    for (int n(0); n != current_file_disk.disk_element_size(); ++n) {
      ordering.insert(std::make_pair(current_file_disk.disk_element(n).data_name(),
                                     current_file_disk.disk_element(n).serialised_value()));
    }
  }

  file_disk.Clear();
  auto itr(ordering.begin());
  for (size_t i(0); i != detail::Parameters::max_file_element_count(); ++i, ++itr) {
    protobuf::DiskStoredElement* element(file_disk.add_disk_element());
    AddDataToElement(itr, element);
  }
  ordering.erase(ordering.begin(), itr);

  content = NonEmptyString(file_disk.SerializeAsString());
  WriteFile(current_path, content.string());
  file_hashes_.at(new_counter) = crypto::Hash<crypto::SHA512>(content);
  boost::filesystem::rename(current_path,
                            GetFilePath(kRoot_, file_hashes_.at(new_counter), new_counter));
  ++new_counter;
}

// Helper functions

boost::filesystem::path DiskBasedStorage::GetFilePath(const boost::filesystem::path& base_path,
                                                      const crypto::SHA512Hash& hash,
                                                      size_t file_number) {
  return base_path / GetFileName(hash, file_number);
}

// This function expects lhs to have data name and value always
bool DiskBasedStorage::MatchingDiskElements(const protobuf::DiskStoredElement& lhs,
                                            const protobuf::DiskStoredElement& rhs) const {
  return lhs.data_name() == rhs.data_name() &&
         (rhs.serialised_value().empty() ? true : lhs.serialised_value() == rhs.serialised_value());
}

}  // namespace vault

}  // namespace maidsafe
