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

DiskBasedStorage::DiskBasedStorage(const fs::path& root) : kRoot_(root), active_(), file_ids_() {
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
  for (; root_itr != end_itr; ++root_itr) {
    try {
      auto file_name_parts(GetAndVerifyFileNameParts(root_itr, most_recent_file_full));
      file_ids_.insert(file_name_parts);
    }
    catch(const std::exception& e) {
      LOG(kWarning) << *root_itr << " is not a valid DiskStoredFile: " << e.what();
      boost::system::error_code ec;
      fs::remove(*root_itr, ec);
    }
  }

  if (file_ids_.empty())
    file_ids_.emplace(0, crypto::SHA512Hash());
  else if (most_recent_file_full)
    file_ids_.emplace((*file_ids_.rbegin()).first + 1, crypto::SHA512Hash());
}

DiskBasedStorage::FileIdentity DiskBasedStorage::GetAndVerifyFileNameParts(
    fs::directory_iterator itr,
    bool& most_recent_file_full) const {
  auto hash(crypto::HashFile<crypto::SHA512>(*itr));
  std::string expected_extension("." + EncodeToBase32(hash));
  if ((*itr).path().extension().string() != expected_extension) {
    LOG(kInfo) << "Contents don't hash to what file name contains.";
    ThrowError(CommonErrors::hashing_error);
  }

  FileIdentity file_id(std::make_pair(std::stoi((*itr).path().stem().string()), hash));
  auto file(ParseFile(file_id));
  if (file.element_size() > detail::Parameters::max_file_element_count()) {
    LOG(kWarning) << *itr << " has too many elements.";
    // TODO(Fraser#5#): 2013-02-14 - Consider different error to indicate probable malicious
    //                               tampering with file contents.
    ThrowError(CommonErrors::parsing_error);
  }

  if (file_id.first > (*file_ids_.rbegin()).first)
    most_recent_file_full = (file.element_size() == detail::Parameters::max_file_element_count());

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
  if (file_ids_.size() == 1U)
    return;

  std::vector<protobuf::DiskStoredElement> elements;
  auto write_itr(GetReorganiseStartPoint(elements));
  if (write_itr == file_ids_.end())
    return;

  // Start copying
  //size_t max_count(static_cast<size_t>(detail::Parameters::max_file_element_count()));
  //auto read_itr(write_itr);
  //++read_itr;
  //std::vector<protobuf::DiskStoredElement> read_elements;
  //for (; read_itr != file_ids_.end(); ++read_itr) {
  //  auto read_file(ParseFile(*read_itr));
  //  for (int i(0); i != read_file.element_size(); ++i)
  //    read_elements.push_back(read_file.element(i));

  //  while (!read_elements.empty()) {
  //    size_t copy_count(std::min(max_count - write_elements.size(), read_elements.size()));
  //    std::move(read_elements.begin(), read_elements.begin() + copy_count,
  //              std::back_inserter(write_elements));
  //    read_elements.erase(read_elements.begin(), read_elements.begin() + copy_count);
  //    if (write_elements.size() == max_count) {
  //      detail::SortElements(write_elements);
  //      write_file.Clear();
  //      for (int i(0); i != detail::Parameters::max_file_element_count(); ++i)
  //        write_file.add_element()->CopyFrom(write_elements[i]);

  //    }
  //  }
  //  copy to write_file

  //}

  //while (reader_itr != file_ids_.end()) {
  //  if (writer.element_size())
  //}

  //erase remove file_ids_

  //OrderingMap ordering;
  //size_t current_counter(1), new_counter(0), file_hashes_max(file_ids_.size());
  //while (current_counter != file_hashes_max) {
  //  fs::path current_path(GetFilePath(kRoot_, file_hashes_.at(new_counter), new_counter));
  //  NonEmptyString current_content(ReadFile(current_path));
  //  protobuf::DiskStoredFile current_file_disk;
  //  current_file_disk.ParseFromString(current_content.string());
  //  HandleNextFile(current_path, current_file_disk, ordering, current_counter, new_counter);
  //}
}

DiskBasedStorage::FileIdentities::iterator DiskBasedStorage::GetReorganiseStartPoint(
    std::vector<protobuf::DiskStoredElement>& elements) {
  // Find first non-full file in order to begin the reorganisation.
  auto write_itr(file_ids_.begin());
  protobuf::DiskStoredFile write_file;
  elements.reserve(detail::Parameters::max_file_element_count());
  for (; write_itr != file_ids_.end(); ++write_itr) {
    write_file = ParseFile(*write_itr);
    if (write_file.element_size() != detail::Parameters::max_file_element_count()) {
      for (int i(0); i != write_file.element_size(); ++i)
        elements.push_back(write_file.element(i));
      break;
    }
  }
  return write_itr;
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

  // If it's the most recent file, check if its full and if so, add empty hash
  if (file.element_size() == detail::Parameters::max_file_element_count() &&
      (*itr).first == (*file_ids_.rbegin()).first) {
    file_ids_.emplace(file_id.first + 1, crypto::SHA512Hash());
  }
}

}  // namespace vault

}  // namespace maidsafe
