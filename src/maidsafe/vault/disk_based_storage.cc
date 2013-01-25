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

#include "maidsafe/vault/utils.h"


namespace fs = boost::filesystem;

namespace maidsafe {

namespace vault {

namespace {

const size_t kNewFileTrigger(10000);
const std::string kEmptyFileHash("empty file hash");

typedef std::promise<NonEmptyString> NonEmptyStringPromise;
typedef std::promise<uint32_t> Uint32tPromise;
typedef std::promise<DiskBasedStorage::PathVector> VectorPathPromise;

void AddDataToElement(const DiskBasedStorage::OrderingMap::reverse_iterator& r_it,
                      protobuf::DiskStoredElement*& disk_element) {
  disk_element->set_data_name((*r_it).first);
  disk_element->set_serialised_value((*r_it).second);
}

void AddElementsToOrdering(const protobuf::DiskStoredFile& current_file_disk,
                           const protobuf::DiskStoredFile& previous_file_disk,
                           DiskBasedStorage::OrderingMap& ordering) {
  for (int c(0); c != current_file_disk.disk_element_size(); ++c) {
    ordering.insert(
        std::make_pair(current_file_disk.disk_element(c).data_name(),
                       current_file_disk.disk_element(c).serialised_value()));
  }
  for (int p(0); p != previous_file_disk.disk_element_size(); ++p) {
    ordering.insert(
        std::make_pair(previous_file_disk.disk_element(p).data_name(),
                       previous_file_disk.disk_element(p).serialised_value()));
  }
}

}  // namespace

class DiskBasedStorage::Changer {
 public:
  Changer() : functor_(nullptr) {}
  Changer(const std::function<void(std::string&)> functor) : functor_(functor) {}

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
  if (fs::exists(root))
    TraverseAndVerifyFiles(root);
  else
    fs::create_directory(root);

  if (file_hashes_.empty())
    file_hashes_.push_back(kEmptyFileHash);
}

void DiskBasedStorage::TraverseAndVerifyFiles(const fs::path& root) {
  std::string hash;
  size_t file_number(std::numeric_limits<size_t>::max());
  fs::directory_iterator root_itr(root), end_itr;
  for (; root_itr != end_itr; ++root_itr) {
    try {
      detail::ExtractElementsFromFilename(fs::path(*root_itr).filename().string(),
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

uint32_t DiskBasedStorage::VerifyFileHashAndCountElements(const std::string& hash,
                                                          size_t file_number) {
  protobuf::DiskStoredFile disk_file;
  fs::path file_path;
  NonEmptyString file_content;
  ReadAndParseFile(hash, file_number, disk_file, file_path, file_content);
  if (EncodeToBase32(crypto::Hash<crypto::SHA512>(file_content)) != hash) {
    LOG(kInfo) << "Contents don't hash to what file name contains.";
    throw std::exception();
  }
  return static_cast<uint32_t>(disk_file.disk_element_size());
}

void DiskBasedStorage::AddToFileData(const std::string& hash,
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
                       file_names.push_back(detail::GetFileName(file_hashes_.at(n), n));
                   }
                   promise->set_value(file_names);
               });
  return std::move(future);
}

std::future<NonEmptyString> DiskBasedStorage::GetFile(const fs::path& path) const {
  // Check path? It might not exist anymore because of operations in the queue
  // std::string hash;
  // size_t file_number;
  // ExtractElementsFromFilename(path.filename().string(), hash, file_number);
  // if (file_data_.at(file_number) != hash)
  //   throw std::exception();
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
  std::string hash;
  detail::ExtractElementsFromFilename(path.string(), hash, file_number);
  if (EncodeToBase32(crypto::Hash<crypto::SHA512>(content)) != hash) {
    LOG(kError) << "Content doesn't hash.";
    throw std::exception();
  }
  active_.Send([path, content, file_number, hash, this] () {
                 DoPutFile(kRoot_ / path, content, file_number, hash);
               });
}

void DiskBasedStorage::DoPutFile(const fs::path& path,
                                 const NonEmptyString& content,
                                 size_t file_number,
                                 const std::string& hash) {
  protobuf::DiskStoredFile disk_file;
  assert(disk_file.ParseFromString(content.string()));
  if (file_number < file_hashes_.size()) {
    std::string old_hash(file_hashes_.at(file_number));
    assert(old_hash != hash && "Hash is the same as it's currently held.");
    file_hashes_.at(file_number) = hash;
    fs::remove(detail::GetFilePath(kRoot_, old_hash, file_number));
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
  file_hashes_.at(latest_file_index) = EncodeToBase32(crypto::Hash<crypto::SHA512>(new_content));
  if (disk_file.disk_element_size() == int(kNewFileTrigger)) {
    // Increment the file
    file_hashes_.push_back(kEmptyFileHash);
  }

  fs::path new_path(detail::GetFilePath(kRoot_,
                                        file_hashes_.at(latest_file_index),
                                        latest_file_index));
  WriteFile(new_path, new_content.string());
  if ((!latest_file_path.empty()) && (latest_file_path != new_path))
    fs::remove(latest_file_path);
  MergeFilesAfterAlteration(latest_file_index);
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
      if (detail::MatchingDiskElements(disk_file.disk_element(n), element)) {
        changer.Execute(disk_file, n);
        UpdateFileAfterModification(it, file_index, disk_file, file_path, reorder);
        return;
      }
    }
  }
}

void DiskBasedStorage::ReadAndParseFile(const std::string& hash,
                                        size_t file_index,
                                        protobuf::DiskStoredFile& disk_file,
                                        fs::path& file_path,
                                        NonEmptyString& file_content) const {
  disk_file.Clear();
  if (hash == kEmptyFileHash) {
    LOG(kInfo) << "Empty file, no need to read.";
    return;
  }
  file_path = detail::GetFilePath(kRoot_, hash, file_index);
  file_content = ReadFile(file_path);
  if (!disk_file.ParseFromString(file_content.string())) {
    LOG(kError) << "Failure to parse file content.";
    throw std::exception();
  }
}

void DiskBasedStorage::UpdateFileAfterModification(std::vector<std::string>::reverse_iterator& it,
                                                   size_t file_index,
                                                   protobuf::DiskStoredFile& disk_file,
                                                   fs::path& file_path,
                                                   bool reorder) {
  try {
    NonEmptyString file_content(NonEmptyString(disk_file.SerializeAsString()));
    *it = EncodeToBase32(crypto::Hash<crypto::SHA512>(file_content));
    WriteFile(file_path, file_content.string());
    fs::path new_path(detail::GetFilePath(kRoot_, *it, file_index));
    fs::rename(file_path, new_path);
    if (reorder)
      MergeFilesAfterAlteration(file_index);
  } catch(std::exception& /*e*/) {
    *it = kEmptyFileHash;
    fs::remove(file_path);
  }
}

void DiskBasedStorage::MergeFilesAfterAlteration(size_t current_index) {
  // Handle single file case
  if (file_hashes_.size() == 1U) {
    LOG(kInfo) << "Just one file. No need to merge.";
    return;
  }

  // Handle edge case
  size_t previous_index(current_index - 1);
  if (current_index == 0) {
    previous_index = 0;
    current_index = 1;
  }

  // Handle empty current file
  if (file_hashes_.at(current_index) == kEmptyFileHash) {
    LOG(kInfo) << "Current file empty. No need to merge.";
    return;
  }

  fs::path current_path(detail::GetFilePath(kRoot_,
                                            file_hashes_.at(current_index),
                                            current_index)),
                         previous_path(detail::GetFilePath(kRoot_,
                                                           file_hashes_.at(previous_index),
                                                           previous_index));
  NonEmptyString current_content(ReadFile(current_path)), previous_content(ReadFile(previous_path));
  protobuf::DiskStoredFile current_file_disk, previous_file_disk;
  current_file_disk.ParseFromString(current_content.string());
  previous_file_disk.ParseFromString(previous_content.string());
  OrderingMap ordering;
  AddElementsToOrdering(current_file_disk, previous_file_disk, ordering);

  current_file_disk.Clear();
  previous_file_disk.Clear();
  size_t total_elements(ordering.size());
  auto r_it(ordering.rbegin());

  // Lower index file
  AddToDiskFile(previous_path, previous_file_disk, r_it, previous_index, 0,
                std::min(ordering.size(), kNewFileTrigger));
  if (total_elements > kNewFileTrigger) {
    // Higher index file
    AddToDiskFile(current_path,
                  current_file_disk,
                  r_it,
                  current_index,
                  kNewFileTrigger,
                  total_elements);
  } else {
    // Higher file will disappear and we need to rename files
    fs::remove(current_path);
    for (size_t n(current_index); n != file_hashes_.size() - 1; ++n) {
      file_hashes_.at(n) = file_hashes_.at(n + 1);
      fs::path old_path(detail::GetFilePath(kRoot_, file_hashes_.at(n), n + 1));
      fs::path new_path(detail::GetFilePath(kRoot_, file_hashes_.at(n), n));
      fs::rename(old_path, new_path);
    }
    file_hashes_.pop_back();
  }
}

void DiskBasedStorage::AddToDiskFile(const fs::path& previous_path,
                                     protobuf::DiskStoredFile& previous_file_disk,
                                     OrderingMap::reverse_iterator& r_it,
                                     size_t file_index,
                                     size_t begin,
                                     size_t end) {
  for (size_t n(begin); n != end; ++n, ++r_it) {
    protobuf::DiskStoredElement* disk_element(previous_file_disk.add_disk_element());
    AddDataToElement(r_it, disk_element);
  }
  NonEmptyString previous_content(previous_file_disk.SerializeAsString());
  std::string previous_hash(EncodeToBase32(crypto::Hash<crypto::SHA512>(previous_content)));
  WriteFile(previous_path, previous_content.string());
  fs::rename(previous_path, detail::GetFilePath(kRoot_, previous_hash, file_index));
  file_hashes_.at(file_index) = previous_hash;
}

}  // namespace vault

}  // namespace maidsafe
