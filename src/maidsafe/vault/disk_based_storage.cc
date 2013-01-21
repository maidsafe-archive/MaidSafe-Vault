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

#include <memory>

#include "boost/filesystem/operations.hpp"

#include "maidsafe/common/utils.h"

#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

namespace {

const size_t kNewFileTrigger(999);
const std::string kEmptyFileHash("empty file hash");

typedef std::promise<NonEmptyString> NonEmptyStringPromise;
typedef std::promise<uint32_t> Uint32tPromise;
typedef std::promise<DiskBasedStorage::PathVector> VectorPathPromise;
namespace d = maidsafe::vault::detail;

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

DiskBasedStorage::DiskBasedStorage(const boost::filesystem::path& root)
    : kRoot_(root),
      active_(),
      file_data_() {
  if (boost::filesystem::exists(root))
    TraverseAndVerifyFiles(root);
  else
    boost::filesystem::create_directories(root);
}

void DiskBasedStorage::TraverseAndVerifyFiles(const boost::filesystem::path& root) {
  boost::filesystem::directory_iterator root_itr(root), end_itr;
  std::string hash;
  size_t file_number(-1);
  for (; root_itr != end_itr; ++root_itr) {
    try {
      d::ExtractElementsFromFilename(boost::filesystem::path(*root_itr).filename().string(),
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
  boost::filesystem::path file_path;
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
  if (element_count >= file_data_.size()) {
    for (uint32_t n(file_data_.size() -1); n != element_count; ++n)
      file_data_.push_back(std::make_pair(0, kEmptyFileHash));
    file_data_.push_back(std::make_pair(element_count, hash));
  } else if (file_data_[file_number].second == hash) {
    LOG(kInfo) << "Already filled file index: " << file_number;
  } else if (file_data_[file_number].second == kEmptyFileHash) {
    file_data_[file_number].first = element_count;
    file_data_[file_number].second = hash;
  } else {
    LOG(kWarning) << "Unexpected content in vector: " << file_number;
  }
}

std::future<uint32_t> DiskBasedStorage::GetFileCount() const {
  std::shared_ptr<Uint32tPromise> promise(std::make_shared<Uint32tPromise>());
  std::future<uint32_t> future(promise->get_future());
  active_.Send([promise, this] () {
                 promise->set_value(static_cast<uint32_t>(file_data_.size()));
               });
  return std::move(future);
}

std::future<DiskBasedStorage::PathVector> DiskBasedStorage::GetFileNames() const {
  std::shared_ptr<VectorPathPromise> promise(std::make_shared<VectorPathPromise>());
  std::future<DiskBasedStorage::PathVector> future(promise->get_future());
  active_.Send([promise, this] () { DoGetFileNames(promise); });
  return std::move(future);
}

std::future<NonEmptyString> DiskBasedStorage::GetFile(const boost::filesystem::path& path) const {
  // Check path? It might not exist anymore because of operations in the queue
  // {
  //   std::lock_guard<std::mutex> guard(file_data_mutex_);
  //   std::string hash;
  //   size_t file_number;
  //   ExtractElementsFromFilename(path.filename().string(), hash, file_number);
  //   if (file_data_[file_number].second != hash)
  //     throw std::exception();
  // }
  std::shared_ptr<NonEmptyStringPromise> promise(std::make_shared<NonEmptyStringPromise>());
  std::future<NonEmptyString> future(promise->get_future());
  active_.Send([path, promise, this] () { promise->set_value(ReadFile(path)); });  // NOLINT (Dan)
  return std::move(future);
}

void DiskBasedStorage::WriteFile(const boost::filesystem::path& path,
                                 const NonEmptyString& content) {
  active_.Send([path, content, this] () { DoWriteFile(path, content); });  // NOLINT (Dan)
}

// File names are index no. + hash of contents
void DiskBasedStorage::DoGetFileNames(std::shared_ptr<VectorPathPromise> promise) const {
  std::vector<boost::filesystem::path> file_names;
  for (size_t n(0); n < file_data_.size(); ++n)
    file_names.push_back(d::GetFilePath(kRoot_, file_data_[n].second, n));
  promise->set_value(file_names);
}

void DiskBasedStorage::DoWriteFile(const boost::filesystem::path& path,
                                   const NonEmptyString& content) {
  std::string filename(path.filename().string());
  size_t file_number;
  std::string hash;
  d::ExtractElementsFromFilename(filename, hash, file_number);
  assert(EncodeToBase32(crypto::Hash<crypto::SHA512>(content)) == hash && "Content doesn't hash.");

  std::vector<FileData>::iterator file_itr = FileExists(file_number);
  if (file_itr != file_data_.end()) {
    std::string old_hash((*file_itr).second);
    assert(old_hash != hash && "Hash is the same as it's currently held.");
    (*file_itr).second = hash;
    boost::filesystem::remove(d::GetFilePath(kRoot_, old_hash, file_number));
  } else {
    file_data_.push_back(std::make_pair(file_number, hash));
  }
  maidsafe::WriteFile(path, content.string());
}

void DiskBasedStorage::AddToLatestFile(const protobuf::DiskStoredElement& element) {
  size_t latest_file_index(file_data_.size() - 1);
  protobuf::DiskStoredFile disk_file;
  boost::filesystem::path latest_file_path;
  NonEmptyString file_content;
  if (file_data_[latest_file_index].second != kEmptyFileHash) {
    ReadAndParseFile(file_data_[latest_file_index].second,
                     latest_file_index,
                     disk_file,
                     latest_file_path,
                     file_content);
  }

  disk_file.add_disk_element()->CopyFrom(element);
  NonEmptyString new_content(disk_file.SerializeAsString());
  file_data_[latest_file_index].second = EncodeToBase32(crypto::Hash<crypto::SHA512>(new_content));
  if (file_data_[latest_file_index].first == kNewFileTrigger) {
    // Increment the file
    file_data_.push_back(std::make_pair(uint32_t(0), kEmptyFileHash));
  } else {
    ++file_data_[latest_file_index].first;
  }

  maidsafe::WriteFile(latest_file_path, new_content.string());
  boost::filesystem::rename(latest_file_path, d::GetFilePath(kRoot_,
                                                             file_data_[latest_file_index].second,
                                                             latest_file_index));
}

void DiskBasedStorage::SearchForAndDeleteEntry(const protobuf::DiskStoredElement& element) {
  Changer changer;
  SearchForEntryAndExecuteOperation(element, changer);
}

void DiskBasedStorage::SearchForAndModifyEntry(const protobuf::DiskStoredElement& element,
                                               const std::function<void(std::string&)>& functor) {
  Changer changer(functor);
  SearchForEntryAndExecuteOperation(element, changer);
}

void DiskBasedStorage::SearchForEntryAndExecuteOperation(const protobuf::DiskStoredElement& element,
                                                         Changer& changer) {
  size_t file_index(file_data_.size() - 1);
  boost::filesystem::path file_path;
  protobuf::DiskStoredFile disk_file;
  NonEmptyString file_content;
  for (auto it(file_data_.rbegin()); it != file_data_.rend(); ++it, --file_index) {
    ReadAndParseFile((*it).second, file_index, disk_file, file_path, file_content);
    for (int n(disk_file.disk_element_size()); n != -1; --n) {
      if (d::MatchingDiskElements(disk_file.disk_element(n), element)) {
        changer.Execute(disk_file, file_index);
        UpdateFileAfterModification(it, file_index, disk_file, file_path);
        return;
      }
    }
  }
}

void DiskBasedStorage::ReadAndParseFile(const std::string& hash,
                                        size_t file_index,
                                        protobuf::DiskStoredFile& disk_file,
                                        boost::filesystem::path& file_path,
                                        NonEmptyString& file_content) const {
  disk_file.Clear();
  file_path = d::GetFilePath(kRoot_, hash, file_index);
  file_content = ReadFile(file_path);
  if (!disk_file.ParseFromString(file_content.string())) {
    LOG(kError) << "Failure to parse file content.";
    throw std::exception();
  }
}

void DiskBasedStorage::UpdateFileAfterModification(std::vector<FileData>::reverse_iterator& it,
                                                   size_t file_index,
                                                   protobuf::DiskStoredFile& disk_file,
                                                   boost::filesystem::path& file_path) {
  NonEmptyString file_content(NonEmptyString(disk_file.SerializeAsString()));
  (*it).second = EncodeToBase32(crypto::Hash<crypto::SHA512>(file_content));
  maidsafe::WriteFile(file_path, file_content.string());
  boost::filesystem::rename(file_path, d::GetFilePath(kRoot_, (*it).second, file_index));
}

std::vector<DiskBasedStorage::FileData>::iterator
    DiskBasedStorage::FileExists(size_t file_number) {
  auto itr = file_data_.begin();
  while (itr != file_data_.end()) {
    if ((*itr).first == file_number)
      return itr;
    ++itr;
  }
  return file_data_.end();
}

}  // namespace vault

}  // namespace maidsafe
