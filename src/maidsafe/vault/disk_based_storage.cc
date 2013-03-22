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
#include <iterator>
#include <limits>

#include "maidsafe/vault/types.h"
#include "maidsafe/vault/utils.h"


namespace fs = boost::filesystem;

namespace maidsafe {

namespace vault {

namespace {

//protobuf::DiskStoredFile ParseAndVerifyMessagedFile(
//    const fs::path& filename,
//    const NonEmptyString& content,
//    const std::pair<int, crypto::SHA512Hash>& file_id) {
//  if (crypto::Hash<crypto::SHA512>(content) != file_id.second) {
//    LOG(kError) << "Received invalid file " << filename << ":  content doesn't hash to name.";
//    ThrowError(CommonErrors::hashing_error);
//  }
//
//  protobuf::DiskStoredFile file;
//  if (!file.ParseFromString(content.string())) {
//    LOG(kError) << "Received invalid file " << filename << ":  doesn't parse.";
//    ThrowError(CommonErrors::parsing_error);
//  }
//
//  return file;
//}
std::multimap<DataNameVariant, int32_t> ConvertFromProtobufElements(
    const protobuf::DiskStoredFile& file) {
  std::multimap<DataNameVariant, int32_t> elements;
  auto itr(std::end(elements));
  for (int i(0); i != file.element_size(); ++i) {
    auto data_name_variant(GetDataNameVariant(static_cast<DataTagValue>(file.element(i).type()),
                                              Identity(file.element(i).name())));
    itr = elements.insert(itr, std::make_pair(data_name_variant, file.element(i).value()));
  }
  return elements;
}

}  // namespace


namespace detail {

void SortElements(std::vector<protobuf::DiskStoredElement>& elements) {
  std::sort(
      std::begin(elements),
      std::end(elements),
      [](const protobuf::DiskStoredElement& lhs, const protobuf::DiskStoredElement& rhs)->bool {
          return (lhs.name() < rhs.name()) ? true :
                 (lhs.name() > rhs.name()) ? false : (lhs.type() < rhs.type());
      });
}

void SortFile(protobuf::DiskStoredFile& file, bool verify_elements) {
  if (verify_elements && file.element_size() > detail::Parameters::max_file_element_count) {
    LOG(kWarning) << "Invalid number of elements (" << file.element_size() << ")";
    ThrowError(CommonErrors::parsing_error);
  }

  std::vector<protobuf::DiskStoredElement> elements;
  elements.reserve(file.element_size());
  for (int i(0); i != file.element_size(); ++i) {
    if (verify_elements) {
      Identity name(file.element(i).name());
      // This throws if the name is invalid
      name.string();
      // This throws if the type is invalid
      GetDataNameVariant(static_cast<DataTagValue>(file.element(i).type()), name);
    }
    elements.push_back(file.element(i));
  }

  SortElements(elements);
  for (int i(0); i != file.element_size(); ++i)
    file.mutable_element(i)->CopyFrom(elements[i]);
}

}  // namespace detail


DiskBasedStorage::RecentOperation::RecentOperation(const DataNameVariant& data_name_variant_in,
                                                   int32_t size_in,
                                                   nfs::DataMessage::Action action_in)
    : data_name_variant(data_name_variant_in),
      size(size_in),
      action(action_in) {}

DiskBasedStorage::RecentOperation::RecentOperation(const RecentOperation& other)
    : data_name_variant(other.data_name_variant),
      size(other.size),
      action(other.action) {}

DiskBasedStorage::RecentOperation& DiskBasedStorage::RecentOperation::operator=(
    const RecentOperation& other) {
  data_name_variant = other.data_name_variant;
  size = other.size;
  action = other.action;
  return *this;
}

DiskBasedStorage::RecentOperation::RecentOperation(RecentOperation&& other)
    : data_name_variant(std::move(other.data_name_variant)),
      size(std::move(other.size)),
      action(std::move(other.action)) {}

DiskBasedStorage::RecentOperation& DiskBasedStorage::RecentOperation::operator=(
    RecentOperation&& other) {
  data_name_variant = std::move(other.data_name_variant);
  size = std::move(other.size);
  action = std::move(other.action);
  return *this;
}



DiskBasedStorage::FileDetails::FileDetails(const ElementNameSubstr& min_element_in,
                                           const ElementNameSubstr& max_element_in,
                                           const crypto::SHA1Hash& hash_in)
    : min_element(min_element_in),
      max_element(max_element_in),
      hash(hash_in) {}

DiskBasedStorage::FileDetails::FileDetails(const FileDetails& other)
    : min_element(other.min_element),
      max_element(other.max_element),
      hash(other.hash) {}

DiskBasedStorage::FileDetails& DiskBasedStorage::FileDetails::operator=(
    const FileDetails& other) {
  min_element = other.min_element;
  max_element = other.max_element;
  hash = other.hash;
  return *this;
}

DiskBasedStorage::FileDetails::FileDetails(FileDetails&& other)
    : min_element(std::move(other.min_element)),
      max_element(std::move(other.max_element)),
      hash(std::move(other.hash)) {}

DiskBasedStorage::FileDetails& DiskBasedStorage::FileDetails::operator=(FileDetails&& other) {
  min_element = std::move(other.min_element);
  max_element = std::move(other.max_element);
  hash = std::move(other.hash);
  return *this;
}



DiskBasedStorage::DiskBasedStorage(const fs::path& root)
    : kRoot_(root),
      current_puts_(),
      current_deletes_(),
      file_ids_() {
  detail::InitialiseDirectory(kRoot_);

  fs::directory_iterator root_itr(kRoot_), end_itr;
  for (; root_itr != end_itr; ++root_itr) {
    try {
      auto file_and_details(ParseFile(fs::path(*root_itr), true));
      auto file_id(std::make_pair(std::stoi((*root_itr).path().stem().string()),
                                  file_and_details.second));
      if (!file_ids_.insert(file_id).second) {
        // TODO(Fraser#5#): 2013-02-14 - Consider different error to indicate probable malicious
        //                               tampering with file contents.
        ThrowError(CommonErrors::parsing_error);
      }
    }
    catch(const std::exception& e) {
      LOG(kWarning) << *root_itr << " is not a valid DiskStoredFile: " << e.what();
      boost::system::error_code ec;
      fs::remove(*root_itr, ec);
    }
  }

  VerifyFileGroup();
}

fs::path DiskBasedStorage::GetFileName(const FileGroup::value_type& file_id) const {
  return fs::path(std::to_string(file_id.first) + "." + EncodeToBase32(file_id.second.hash));
}

std::pair<protobuf::DiskStoredFile, DiskBasedStorage::FileDetails> DiskBasedStorage::ParseFile(
    const FileGroup::value_type& file_id,
    bool verify) const {
  fs::path file_path(kRoot_ / GetFileName(file_id));
  return ParseFile(file_path, verify);
}

std::pair<protobuf::DiskStoredFile, DiskBasedStorage::FileDetails> DiskBasedStorage::ParseFile(
    const fs::path& file_path,
    bool verify) const {
  protobuf::DiskStoredFile file;
  std::string contents(ReadFile(file_path).string());
  if (!file.ParseFromString(contents)) {
    LOG(kError) << "Failure to parse " << file_path << " content.";
    ThrowError(CommonErrors::parsing_error);
  }

  if (file.element_size() == 0) {
    LOG(kWarning) << file_path << " has no elements (" << file.element_size() << ")";
    // TODO(Fraser#5#): 2013-02-14 - Consider different error to indicate probable malicious
    //                               tampering with file contents.
    ThrowError(CommonErrors::parsing_error);
  }

  // If the filename doesn't contain a dot, extension() returns an empty path causing substr() to
  // throw a std::out_of_range exception.
  crypto::SHA1Hash hash(DecodeFromBase32(file_path.extension().string().substr(1)));
  FileDetails file_details(
      FileDetails::ElementNameSubstr(file.element(0).name().substr(0, FileDetails::kSubstrSize)),
      FileDetails::ElementNameSubstr(
          file.element(file.element_size() - 1).name().substr(0, FileDetails::kSubstrSize)),
      hash);
  auto file_and_details(std::make_pair(file, file_details));
  if (verify)
    VerifyFile(file_and_details);

  return file_and_details;
}

void DiskBasedStorage::VerifyFile(
    std::pair<protobuf::DiskStoredFile, FileDetails>& file_and_details) const {
  // TODO(Fraser#5#): 2013-02-14 - Consider different errors here to indicate probable malicious
  //                               tampering with file contents.
  detail::SortFile(file_and_details.first, true);
  std::string contents(file_and_details.first.SerializeAsString());
  if (crypto::Hash<crypto::SHA1>(contents) != file_and_details.second.hash) {
    LOG(kWarning) << "Contents don't hash to what file name contains.";
    ThrowError(CommonErrors::hashing_error);
  }
}

void DiskBasedStorage::VerifyFileGroup() const {
  if (file_ids_.empty())
    return;
  auto itr(std::begin(file_ids_)), next(std::begin(file_ids_));
  while (itr != std::end(file_ids_)) {
    if ((*itr).second.max_element < (*itr).second.min_element)
      ThrowError(CommonErrors::parsing_error);
    ++next;
    if (next != std::end(file_ids_) && (*next).second.min_element < (*itr).second.max_element)
      ThrowError(CommonErrors::parsing_error);
    ++itr;
  }
}

void DiskBasedStorage::ApplyRecentOperations(const std::vector<RecentOperation>& recent_ops) {
  SetCurrentOps(recent_ops);
  auto itr(GetReorganiseStartPoint());
  Elements temp;
  if (itr != std::end(file_ids_))
    temp = ConvertFromProtobufElements(ParseFile(*itr, false).first);

  while (itr != std::end(file_ids_)) {
    ++itr;
  }

  //open file 0 (n == 0)
  //iterate {
  //  open file n+1
  //  apply to put map
  //  remove delete elements
  //  write file n
  //  if <1000 exit loop
  //}
  //write last file
}

void DiskBasedStorage::SetCurrentOps(const std::vector<RecentOperation>& recent_ops) {
  auto delete_begin_itr(std::partition(std::begin(recent_ops),
                                       std::end(recent_ops),
                                       [](const RecentOperation& op)->bool {
                                           assert(op.action == nfs::DataMessage::Action::kPut ||
                                                  op.action == nfs::DataMessage::Action::kDelete);
                                           return op.action == nfs::DataMessage::Action::kPut;
                                       }));
  current_puts_.clear();
  current_deletes_.clear();
  auto itr(std::begin(recent_ops));
  for (; itr != delete_begin_itr; ++itr)
    current_puts_.emplace((*itr).data_name_variant, (*itr).size);
  for (; itr != std::end(recent_ops); ++itr)
    current_deletes_.emplace((*itr).data_name_variant, (*itr).size);
}

DiskBasedStorage::FileGroup::iterator DiskBasedStorage::GetReorganiseStartPoint() {
  DataNameVariant min_element;
  if (current_puts_.empty()) {
    if (current_deletes_.empty())
      return std::end(file_ids_);
    else
      min_element = (*std::begin(current_deletes_)).first;
  } else if (current_deletes_.empty()) {
    min_element = (*std::begin(current_puts_)).first;
  } else {
    min_element = std::min((*std::begin(current_puts_)).first,
                           (*std::begin(current_deletes_)).first);
  }

  GetIdentityVisitor get_identity;
  FileDetails::ElementNameSubstr min_element_name(
      min_element.apply_visitor(get_identity).string().substr(0, FileDetails::kSubstrSize));
  auto lower(std::lower_bound(std::begin(file_ids_),
                              std::end(file_ids_),
                              min_element_name,
                              [](const FileGroup::value_type& file_id,
                                 const FileDetails::ElementNameSubstr& min_name) {
                                return file_id.second.max_element < min_name;
                              }));
  return (lower == std::end(file_ids_) && !file_ids_.empty()) ? --lower : lower;
}





void DiskBasedStorage::DeleteEntry(int index, protobuf::DiskStoredFile& file) const {
  protobuf::DiskStoredFile temp;
  for (int i(0); i != file.element_size(); ++i) {
    if (i != index) {
      temp.add_element()->CopyFrom(file.element(i));
      break;
    }
  }
  file = temp;
}

void DiskBasedStorage::SaveChangedFile(const FileIdentity& file_id,
                                       const protobuf::DiskStoredFile& file) {
  auto itr(file_ids_.find(file_id.first));
  if (itr == std::end(file_ids_)) {
    LOG(kError) << "Failed to find file " << file_id.first << " in index.";
    ThrowError(CommonErrors::invalid_parameter);
  }

  NonEmptyString new_content(file.SerializeAsString());
  auto new_hash(crypto::Hash<crypto::SHA512>(new_content));
  fs::path new_path(kRoot_ / GetFileName(std::make_pair(file_id.first, new_hash)));
  if (!WriteFile(new_path, new_content.string()))
    ThrowError(CommonErrors::filesystem_io_error);

  // If this is not the first entry in a new file, the file_id hash will be initialised.
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

void DiskBasedStorage::ReadIntoMemory(FileIdentities::iterator &read_itr,
                                      std::vector<protobuf::DiskStoredElement>& elements) {
  while (read_itr != std::end(file_ids_) &&
         static_cast<int>(elements.size()) < 2 * detail::Parameters::max_file_element_count) {
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
                        static_cast<size_t>(detail::Parameters::max_file_element_count)));
  for (size_t i(0); i != count; ++i)
    write_file.add_element()->CopyFrom(elements[i]);

  elements.erase(std::begin(elements), std::begin(elements) + count);
  SaveChangedFile(*write_itr, write_file);
  ++write_itr;
}

void DiskBasedStorage::PruneFilesToEnd(const FileIdentities::iterator& first_itr) {
  FileIdentities::iterator itr(first_itr);
  while (itr != std::end(file_ids_)) {
    fs::path file_path(kRoot_ / GetFileName(*itr));
    boost::system::error_code ec;
    if (!fs::remove(file_path, ec))
      LOG(kWarning) << "Failed to remove " << file_path << " during prune: " << ec.message();
    ++itr;
  }
  file_ids_.erase(first_itr, std::end(file_ids_));
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
  if (itr == std::end(file_ids_)) {
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
}

}  // namespace vault

}  // namespace maidsafe
