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

#include "maidsafe/vault/types.h"

namespace fs = boost::filesystem;

namespace maidsafe {

namespace vault {

namespace {


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
    ThrowError(CommonErrors::unexpected_filename_format);
  }
  file_number = static_cast<size_t>(std::stoi(std::string(filename.begin(), it)));
  hash = crypto::SHA512Hash(DecodeFromBase32(std::string(it + 1, filename.end())));
}

boost::filesystem::path GetFileName(const crypto::SHA512Hash& hash, size_t file_number) {
  return boost::filesystem::path(std::to_string(file_number) + "." + EncodeToBase32(hash));
}

}  // namespace

DiskBasedStorage::DiskBasedStorage(const boost::filesystem::path& root)
    : kRoot_(root),
      active_(),
      file_hashes_(),
      kEmptyFileHash_("filehashfilehashfilehashfilehashfilehashfilehashfilehashfilehash") {
  if (boost::filesystem::exists(root)) {
    if (boost::filesystem::is_directory(root))
      TraverseAndVerifyFiles(root);
    else
      ThrowError(CommonErrors::not_a_directory);
  } else {
    boost::filesystem::create_directory(root);
  }

  if (file_hashes_.empty())
    file_hashes_.push_back(kEmptyFileHash_);
}

void DiskBasedStorage::TraverseAndVerifyFiles(const boost::filesystem::path& root) {
  crypto::SHA512Hash hash;
  size_t file_number(std::numeric_limits<size_t>::max());
  boost::filesystem::directory_iterator root_itr(root), end_itr;
  for (; root_itr != end_itr; ++root_itr) {
    try {
      ExtractElementsFromFilename(boost::filesystem::path(*root_itr).filename().string(),
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
  boost::filesystem::path file_path;
  NonEmptyString file_content;
  ReadAndParseFile(hash, file_number, disk_file, file_path, file_content);
  if (crypto::Hash<crypto::SHA512>(file_content) != hash) {
    LOG(kInfo) << "Contents don't hash to what file name contains.";
    ThrowError(CommonErrors::hashing_error);
  }
  return static_cast<uint32_t>(disk_file.disk_element_size());
}

void DiskBasedStorage::AddToFileData(const crypto::SHA512Hash& hash,
                                     size_t file_number,
                                     uint32_t element_count) {
  if (element_count >= file_hashes_.size()) {
    file_hashes_.resize(element_count - 1, kEmptyFileHash_);
    file_hashes_.push_back(hash);
  } else if (file_hashes_.at(file_number) == hash) {
    LOG(kInfo) << "Already filled file index: " << file_number;
  } else if (file_hashes_.at(file_number) == kEmptyFileHash_) {
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
                   std::vector<boost::filesystem::path> file_names;
                   for (size_t n(0); n < file_hashes_.size(); ++n) {
                     if (file_hashes_.at(n) != kEmptyFileHash_)
                       file_names.push_back(GetFileName(file_hashes_.at(n), n));
                   }
                   promise->set_value(file_names);
               });
  return std::move(future);
}

std::future<NonEmptyString> DiskBasedStorage::GetFile(
    const boost::filesystem::path& filename) const {
  // Check path? It might not exist anymore because of operations in the queue
  // crypto::SHA512Hash hash
  // size_t file_number;
  // ExtractElementsFromFilename(path.filename().string(), hash, file_number);
  // if (file_data_.at(file_number) != hash)
  //   ThrowError(CommonErrors::hashing_error);
  std::shared_ptr<NonEmptyStringPromise> promise(std::make_shared<NonEmptyStringPromise>());
  std::future<NonEmptyString> future(promise->get_future());
  active_.Send([filename, promise, this] () {
                 try {
                   promise->set_value(ReadFile(kRoot_ / filename));
                 }
                 catch(...) {
                   promise->set_exception(std::current_exception());
                 }
               });
  return std::move(future);
}

void DiskBasedStorage::PutFile(const boost::filesystem::path& filename,
                               const NonEmptyString& content) {
  size_t file_number;
  crypto::SHA512Hash hash;
  ExtractElementsFromFilename(filename.string(), hash, file_number);
  if (crypto::Hash<crypto::SHA512>(content) != hash) {
    LOG(kError) << "Content doesn't hash.";
    ThrowError(CommonErrors::hashing_error);
  }
  active_.Send([filename, content, file_number, hash, this] () {
                 DoPutFile(kRoot_ / filename, content, file_number, hash);
               });
}

void DiskBasedStorage::DoPutFile(const boost::filesystem::path& path,
                                 const NonEmptyString& content,
                                 size_t file_number,
                                 const crypto::SHA512Hash& hash) {
  protobuf::DiskStoredFile disk_file;
  assert(disk_file.ParseFromString(content.string()));
  if (file_number < file_hashes_.size()) {
    crypto::SHA512Hash old_hash(file_hashes_.at(file_number));
    assert(old_hash != hash && "Hash is the same as it's currently held.");
    WriteFile(path, content.string());
    try {
      boost::filesystem::remove(GetFilePath(kRoot_, old_hash, file_number));
    }
    catch(const std::exception& e) {
      LOG(kError) << "Failed to remove the old file asociated with index: " << file_number
                  << ": " << e.what();
      boost::filesystem::remove(path);
      ThrowError(CommonErrors::filesystem_io_error);
    }
    file_hashes_.at(file_number) = hash;
  } else {
    size_t file_hashes_size(file_hashes_.size());
    while (file_number > file_hashes_size)
      file_hashes_.push_back(kEmptyFileHash_);
    try {
      WriteFile(path, content.string());
      file_hashes_.push_back(hash);
    }
    catch(const std::exception& e) {
      LOG(kError) << "failure writing new file at index: " << file_number << ": " << e.what();
      file_hashes_.resize(file_hashes_size);
      ThrowError(CommonErrors::filesystem_io_error);
    }
  }
}

void DiskBasedStorage::ReadAndParseFile(const crypto::SHA512Hash& hash,
                                        size_t file_index,
                                        protobuf::DiskStoredFile& disk_file,
                                        boost::filesystem::path& file_path,
                                        NonEmptyString& file_content) const {
  disk_file.Clear();
  if (hash == kEmptyFileHash_) {
    LOG(kInfo) << "Empty file, no need to read.";
    return;
  }
  file_path = GetFilePath(kRoot_, hash, file_index);
  file_content = ReadFile(file_path);
  if (!disk_file.ParseFromString(file_content.string())) {
    LOG(kError) << "Failure to parse file content.";
    ThrowError(CommonErrors::parsing_error);
  }
}

void DiskBasedStorage::UpdateFileAfterModification(
    std::vector<crypto::SHA512Hash>::reverse_iterator& it,
    size_t file_index,
    const protobuf::DiskStoredFile& disk_file,
    const boost::filesystem::path& file_path,
    bool reorder) {
  try {
    NonEmptyString file_content(NonEmptyString(disk_file.SerializeAsString()));
    *it = crypto::Hash<crypto::SHA512>(file_content);
    WriteFile(file_path, file_content.string());
    boost::filesystem::rename(file_path, GetFilePath(kRoot_, *it, file_index));
    if (reorder &&
        disk_file.disk_element_size() < int(detail::Parameters::min_file_element_count())) {
      MergeFilesAfterDelete();
    }
  } catch(const std::exception& /*e*/) {
    *it = kEmptyFileHash_;
    boost::filesystem::remove(file_path);
  }
}

void DiskBasedStorage::MergeFilesAfterDelete() {
  if (CheckSpecialMergeCases())
    return;

  OrderingMap ordering;
  size_t current_counter(1), new_counter(0), file_hashes_max(file_hashes_.size());
  while (current_counter != file_hashes_max) {
    boost::filesystem::path current_path(GetFilePath(kRoot_, file_hashes_.at(new_counter), new_counter));
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

// static Helper function
boost::filesystem::path DiskBasedStorage::GetFilePath(const boost::filesystem::path& base_path,
                                                      const crypto::SHA512Hash& hash,
                                                      size_t file_number) {
  return base_path / GetFileName(hash, file_number);
}


}  // namespace vault

}  // namespace maidsafe
