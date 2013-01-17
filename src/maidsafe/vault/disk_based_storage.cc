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

namespace maidsafe {

namespace vault {

namespace {

void ExtractElementsFromFilename(const std::string& filename,
                                 std::string& hash,
                                 size_t& file_number) {
  auto it(std::find(filename.begin(), filename.end(), '.'));
  if (it == filename.end()) {
    LOG(kError) << "No dot in the file name.";
    throw std::exception();
  }
  hash = std::string(filename.begin(), it - 1);
  file_number = static_cast<size_t>(std::stoi(std::string(it + 1, filename.end())));
}

boost::filesystem::path GetFilePath(const boost::filesystem::path& base_path,
                                    const std::string& hash,
                                    size_t file_number) {
  return base_path / (hash + "." + std::to_string(file_number));
}

void RemoveElement(protobuf::DiskStoredFile& disk_file, int index_to_remove) {
  protobuf::DiskStoredFile temp;
  for (int n(0); n != disk_file.disk_element_size(); ++n) {
    if (n != index_to_remove)
      temp.add_disk_element()->CopyFrom(disk_file.disk_element(n));
  }

  disk_file = temp;
}

}  // namespace

const size_t kNewFileTrigger(999);
const std::string kEmptyFileHash("empty file hash");
typedef std::promise<NonEmptyString> NonEmptyStringPromise;

DiskBasedStorage::DiskBasedStorage(const boost::filesystem::path& root)
    : kRoot_(root),
      active_(),
      file_data_(),
      file_data_mutex_() {}

uint32_t DiskBasedStorage::GetFileCount() const {
  std::lock_guard<std::mutex> guard(file_data_mutex_);
  return static_cast<uint32_t>(file_data_.size());
}

// File names are index no. + hash of contents
std::vector<boost::filesystem::path> DiskBasedStorage::GetFileNames() const {
  std::vector<boost::filesystem::path> file_names;
  {
    std::lock_guard<std::mutex> guard(file_data_mutex_);
    for (size_t n(0); n < file_data_.size(); ++n)
      file_names.push_back(GetFilePath(kRoot_, file_data_[n].second, n));
  }
  return std::move(file_names);
}

void DiskBasedStorage::WriteFile(const boost::filesystem::path& path,
                                 const NonEmptyString& content) {
  std::string filename(path.filename().string());
  size_t file_number;
  std::string hash;
  ExtractElementsFromFilename(filename, hash, file_number);
  assert(EncodeToBase32(crypto::Hash<crypto::SHA512>(content)) == hash && "Content doesn't hash.");
  std::string old_hash;
  {
    std::lock_guard<std::mutex> guard(file_data_mutex_);
    assert(file_data_[file_number].second != hash && "Hash is the same as it's currently held.");
    old_hash = file_data_[file_number].second;
    file_data_[file_number].second = hash;
  }
  active_.Send([path, content, file_number, old_hash, this] () {
                 maidsafe::WriteFile(path, content.string());
                 boost::filesystem::remove(GetFilePath(kRoot_, old_hash, file_number));
               });
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

void DiskBasedStorage::AddToLatestFile(const protobuf::DiskStoredElement& element) {
  std::lock_guard<std::mutex> guard(file_data_mutex_);
  size_t latest_file_index(file_data_.size() - 1);
  protobuf::DiskStoredFile disk_file;
  boost::filesystem::path latest_file_path;
  if (file_data_[latest_file_index].second != kEmptyFileHash) {
    latest_file_path = GetFilePath(kRoot_, file_data_[latest_file_index].second, latest_file_index);
    NonEmptyString file_content(ReadFile(latest_file_path));
    if (!disk_file.ParseFromString(file_content.string())) {
      LOG(kError) << "Failure to parse file content.";
      throw std::exception();
    }
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

  WriteFile(latest_file_path, new_content);
  boost::filesystem::rename(latest_file_path, GetFilePath(kRoot_,
                                                          file_data_[latest_file_index].second,
                                                          latest_file_index));
}

void DiskBasedStorage::SearchAndDeleteEntry(const protobuf::DiskStoredElement& element) {
  std::lock_guard<std::mutex> guard(file_data_mutex_);
  size_t file_index(file_data_.size() - 1);
  boost::filesystem::path file_path;
  protobuf::DiskStoredFile disk_file;
  NonEmptyString file_content;
  for (auto it(file_data_.rbegin()); it != file_data_.rend(); ++it, --file_index) {
    disk_file.Clear();
    file_path = GetFilePath(kRoot_, (*it).second, file_index);
    file_content = ReadFile(file_path);
    if (!disk_file.ParseFromString(file_content.string())) {
      LOG(kError) << "Failure to parse file content.";
      throw std::exception();
    }

    for (int n(disk_file.disk_element_size()); n != -1; --n) {
      if (disk_file.disk_element(n).data_name() == element.data_name() &&
          disk_file.disk_element(n).version() == element.version()) {
        RemoveElement(disk_file, n);
        file_content = NonEmptyString(disk_file.SerializeAsString());
        (*it).second = EncodeToBase32(crypto::Hash<crypto::SHA512>(file_content));
        WriteFile(file_path, new_content);
        boost::filesystem::rename(file_path, GetFilePath(kRoot_, (*it).second, file_index));
        return;
      }
    }
  }
}

}  // namespace vault

}  // namespace maidsafe
