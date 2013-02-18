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

#include <iterator>
#include <limits>

#include "maidsafe/vault/types.h"


namespace fs = boost::filesystem;

namespace maidsafe {

namespace vault {

namespace {

protobuf::DiskStoredFile ParseAndVerifyMessagedFile(
    const fs::path& filename,
    const NonEmptyString& content,
    const std::pair<int, crypto::SHA512Hash>& file_id) {
  if (crypto::Hash<crypto::SHA512>(content) != file_id.second) {
    LOG(kError) << "Received invalid file " << filename << ":  content doesn't hash to name.";
    ThrowError(CommonErrors::hashing_error);
  }

  protobuf::DiskStoredFile file;
  if (!file.ParseFromString(content.string())) {
    LOG(kError) << "Received invalid file " << filename << ":  doesn't parse.";
    ThrowError(CommonErrors::parsing_error);
  }

  return file;
}

}  // namespace


namespace detail {

void SortElements(std::vector<protobuf::DiskStoredElement>& elements) {
  std::sort(
      elements.begin(),
      elements.end(),
      [](const protobuf::DiskStoredElement& lhs, const protobuf::DiskStoredElement& rhs)->bool {
          return (lhs.name() < rhs.name()) ? true :
                 (lhs.name() > rhs.name()) ? false : (lhs.type() < rhs.type());
      });
}

void SortFile(protobuf::DiskStoredFile& file) {
  assert(file.element_size() == detail::Parameters::max_file_element_count());
  std::vector<protobuf::DiskStoredElement> elements;
  elements.reserve(file.element_size());
  for (int i(0); i != file.element_size(); ++i)
    elements.push_back(file.element(i));
  SortElements(elements);
  for (int i(0); i != file.element_size(); ++i)
    file.mutable_element(i)->CopyFrom(elements[i]);
}

}  // namespace detail


DiskBasedStorage::DiskBasedStorage(const fs::path& root)
    : kRoot_(root),
      active_(),
      file_ids_(),
      oldest_non_full_file_index_(std::numeric_limits<int>::max()) {
  if (fs::exists(root)) {
    if (fs::is_directory(root))
      TraverseAndVerifyFiles();
    else
      ThrowError(CommonErrors::not_a_directory);
  } else {
    fs::create_directory(root);
  }
}

void DiskBasedStorage::TraverseAndVerifyFiles() {
  assert(file_ids_.empty());
  fs::directory_iterator root_itr(kRoot_), end_itr;
  bool most_recent_file_full(false);
  int oldest_non_full_file_index(oldest_non_full_file_index_);
  for (; root_itr != end_itr; ++root_itr) {
    try {
      auto file_name_parts(GetAndVerifyFileNameParts(root_itr, most_recent_file_full,
                                                     oldest_non_full_file_index));
      if (!file_ids_.insert(file_name_parts).second) {
        // TODO(Fraser#5#): 2013-02-14 - Consider different error to indicate probable malicious
        //                               tampering with file contents.
        ThrowError(CommonErrors::parsing_error);
      }
      oldest_non_full_file_index_ = oldest_non_full_file_index;
    }
    catch(const std::exception& e) {
      LOG(kWarning) << *root_itr << " is not a valid DiskStoredFile: " << e.what();
      boost::system::error_code ec;
      fs::remove(*root_itr, ec);
    }
  }

  if (file_ids_.empty())
    file_ids_.insert(std::make_pair(0, std::move(crypto::SHA512Hash())));
  else if (most_recent_file_full)
    file_ids_.insert(std::make_pair((*file_ids_.rbegin()).first + 1,
                                    std::move(crypto::SHA512Hash())));
}

DiskBasedStorage::FileIdentity DiskBasedStorage::GetAndVerifyFileNameParts(
    fs::directory_iterator itr,
    bool& most_recent_file_full,
    int& oldest_non_full_file_index) const {
  auto hash(crypto::HashFile<crypto::SHA512>(*itr));
  std::string expected_extension("." + EncodeToBase32(hash));
  if ((*itr).path().extension().string() != expected_extension) {
    LOG(kInfo) << "Contents don't hash to what file name contains.";
    ThrowError(CommonErrors::hashing_error);
  }

  FileIdentity file_id(std::make_pair(std::stoi((*itr).path().stem().string()), hash));
  auto file(ParseFile(file_id));
  if (file.element_size() > detail::Parameters::max_file_element_count() ||
      file_id.first < 0) {
    LOG(kWarning) << *itr << " has too many elements (" << file.element_size() << ") or index ("
                  << file_id.first << ") < 0";
    // TODO(Fraser#5#): 2013-02-14 - Consider different error to indicate probable malicious
    //                               tampering with file contents.
    ThrowError(CommonErrors::parsing_error);
  }

  bool full(file.element_size() == detail::Parameters::max_file_element_count());
  if (file_id.first > (*file_ids_.rbegin()).first)
    most_recent_file_full = full;
  if (!full && file_id.first < oldest_non_full_file_index)
    oldest_non_full_file_index = file_id.first;

  return file_id;
}

fs::path DiskBasedStorage::GetFileName(const FileIdentity& file_id) const {
  return fs::path(std::to_string(file_id.first) + "." + EncodeToBase32(file_id.second));
}

protobuf::DiskStoredFile DiskBasedStorage::ParseFile(const FileIdentity& file_id) const {
  fs::path file_path(kRoot_ / GetFileName(file_id));
  protobuf::DiskStoredFile file;
  std::string content(ReadFile(file_path).string());
  if (!file.ParseFromString(content)) {
    LOG(kError) << "Failure to parse file content.";
    ThrowError(CommonErrors::parsing_error);
  }
  return file;
}

void DiskBasedStorage::DeleteEntry(int index, protobuf::DiskStoredFile& file) const {
  protobuf::DiskStoredFile temp;
  for (int i(0); i != file.element_size(); ++i) {
    if (i != index)
      temp.add_element()->CopyFrom(file.element(i));
  }
  file = temp;
}

void DiskBasedStorage::SaveChangedFile(const FileIdentity& file_id,
                                       const protobuf::DiskStoredFile& file) {
  auto itr(file_ids_.find(file_id.first));
  if (itr == file_ids_.end()) {
    LOG(kError) << "Failed to find file " << file_id.first << " in index.";
    ThrowError(CommonErrors::invalid_parameter);
  }

  NonEmptyString new_content(file.SerializeAsString());
  auto new_hash(crypto::Hash<crypto::SHA512>(new_content));
  fs::path new_path(kRoot_ / GetFileName(std::make_pair(file_id.first, new_hash)));
  if (!WriteFile(new_path, new_content.string()))
    ThrowError(CommonErrors::filesystem_io_error);

  // If this is the first entry in a new file, the file_id hash will be empty.
  if (file_id.second.IsInitialised()) {
    fs::path old_path(kRoot_ / GetFileName(file_id));
    boost::system::error_code ec;
    if (!boost::filesystem::remove(old_path, ec)) {
      LOG(kError) << "Failed to remove " << old_path << ":  " << ec.message();
      // Remove new file to achieve strong exception guarantee
      boost::filesystem::remove(new_path, ec);
      ThrowError(CommonErrors::filesystem_io_error);
    }
  }

  (*itr).second = new_hash;
}

void DiskBasedStorage::ReorganiseFiles() {
  auto read_itr(GetReorganiseStartPoint());
  if (read_itr == file_ids_.end())
    return;

  auto write_itr(read_itr);
  std::vector<protobuf::DiskStoredElement> elements;
  elements.reserve(3 * detail::Parameters::max_file_element_count());
  while (read_itr != file_ids_.end() && !elements.empty()) {
    ReadIntoMemory(read_itr, elements);
    WriteToDisk(write_itr, elements);
  }

  PruneFilesToEnd(write_itr);
  oldest_non_full_file_index_ = std::numeric_limits<int>::max();
}

DiskBasedStorage::FileIdentities::iterator DiskBasedStorage::GetReorganiseStartPoint() {
  // Find first non-full file in order to begin the reorganisation.  Don't consider the last file.
  if (oldest_non_full_file_index_ >= (*file_ids_.rbegin()).first)
    return file_ids_.end();

  auto write_itr(file_ids_.begin());
  std::advance(write_itr, oldest_non_full_file_index_);
  return write_itr;
}

void DiskBasedStorage::ReadIntoMemory(FileIdentities::iterator &read_itr,
                                      std::vector<protobuf::DiskStoredElement>& elements) {
  while (read_itr != file_ids_.end() &&
         static_cast<int>(elements.size()) < 2 * detail::Parameters::max_file_element_count()) {
    auto read_file(ParseFile(*read_itr));
    for (int i(0); i != read_file.element_size(); ++i)
      elements.push_back(read_file.element(i));
    ++read_itr;
  }
}

void DiskBasedStorage::WriteToDisk(FileIdentities::iterator &write_itr,
                                   std::vector<protobuf::DiskStoredElement>& elements) {
  detail::SortElements(elements);
  protobuf::DiskStoredFile write_file;
  size_t count(std::min(elements.size(),
                        static_cast<size_t>(detail::Parameters::max_file_element_count())));
  for (size_t i(0); i != count; ++i)
    write_file.add_element()->CopyFrom(elements[i]);

  elements.erase(elements.begin(), elements.begin() + count);
  SaveChangedFile(*write_itr, write_file);
  ++write_itr;
}

void DiskBasedStorage::PruneFilesToEnd(const FileIdentities::iterator& first_itr) {
  FileIdentities::iterator itr(first_itr);
  while (itr != file_ids_.end()) {
    fs::path file_path(kRoot_ / GetFileName(*itr));
    boost::system::error_code ec;
    if (!fs::remove(file_path, ec))
      LOG(kWarning) << "Failed to remove " << file_path << " during prune: " << ec.message();
    ++itr;
  }
  file_ids_.erase(first_itr, file_ids_.end());
}

std::future<uint32_t> DiskBasedStorage::GetFileCount() const {
  auto promise(std::make_shared<std::promise<uint32_t>>());
  active_.Send([promise, this] { promise->set_value(static_cast<uint32_t>(file_ids_.size())); });
  return promise->get_future();
}

std::future<std::vector<fs::path>> DiskBasedStorage::GetFileNames() const {
  auto promise(std::make_shared<std::promise<std::vector<fs::path>>>());
  active_.Send([promise, this] {
                   std::vector<fs::path> file_names;
                   file_names.reserve(file_ids_.size());
                   for (auto& file_id : file_ids_)
                     file_names.push_back(GetFileName(file_id));
                   promise->set_value(file_names);
               });
  return promise->get_future();
}

std::future<NonEmptyString> DiskBasedStorage::GetFile(const fs::path& filename) const {
  auto promise(std::make_shared<std::promise<NonEmptyString>>());
  active_.Send([filename, promise, this] {
                   NonEmptyString content(ReadFile(kRoot_ / filename));
                   if (content.IsInitialised()) {
                     promise->set_value(content);
                   } else {
                     promise->set_exception(
                         std::make_exception_ptr(MakeError(CommonErrors::no_such_element)));
                   }
               });
  return promise->get_future();
}

void DiskBasedStorage::PutFile(const fs::path& filename, const NonEmptyString& content) {
  try {
    std::string extension(filename.extension().string().substr(1));
    crypto::SHA512Hash hash(DecodeFromBase32(extension));
    int index(std::stoi(filename.stem().string()));
    FileIdentity file_id(std::make_pair(index, hash));
    active_.Send([filename, content, file_id, this] { DoPutFile(filename, content, file_id); });
  }
  catch(const std::exception& e) {
    LOG(kError) << "Received invalid file " << filename << ":  " << e.what();
  }
}

void DiskBasedStorage::DoPutFile(const fs::path& filename,
                                 const NonEmptyString& content,
                                 const FileIdentity& file_id) {
  protobuf::DiskStoredFile file(ParseAndVerifyMessagedFile(filename, content, file_id));

  auto itr(file_ids_.find(file_id.first));
  if (itr == file_ids_.end()) {
    // This is a new file.
    if (!WriteFile(kRoot_ / filename, content.string()))
      ThrowError(CommonErrors::filesystem_io_error);
    auto result(file_ids_.insert(file_id));
    assert(result.second);
    itr = result.first;
  } else {
    // This is a replacement file.
    SaveChangedFile(*itr, file);
  }

  // If it's full & is the most recent file, add empty hash
  if (file.element_size() == detail::Parameters::max_file_element_count()) {
    if ((*itr).first == (*file_ids_.rbegin()).first)
      file_ids_.insert(std::make_pair(file_id.first + 1, std::move(crypto::SHA512Hash())));
  } else {  // If it's not full, check if it's the oldest non-full.
    if ((*itr).first < oldest_non_full_file_index_)
      oldest_non_full_file_index_ = (*itr).first;
  }
}

}  // namespace vault

}  // namespace maidsafe
