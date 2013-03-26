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

#include "maidsafe/common/on_scope_exit.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/vault/parameters.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/utils.h"


namespace fs = boost::filesystem;

namespace maidsafe {

namespace vault {

namespace {

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

void MergeFromProtobufElements(const protobuf::DiskStoredFile& file,
                               std::multimap<DataNameVariant, int32_t>& elements) {
  auto itr(std::end(elements));
  for (int i(0); i != file.element_size(); ++i) {
    auto data_name_variant(GetDataNameVariant(static_cast<DataTagValue>(file.element(i).type()),
                                              Identity(file.element(i).name())));
    itr = elements.insert(itr, std::make_pair(data_name_variant, file.element(i).value()));
  }
}

void RemoveIntersection(std::multimap<DataNameVariant, int32_t>& lhs,
                        std::multimap<DataNameVariant, int32_t>& rhs) {
  auto lhs_itr(std::begin(lhs));
  auto rhs_itr(std::begin(rhs));
  while (lhs_itr != std::end(lhs) && rhs_itr != std::end(rhs)) {
    if (*lhs_itr < *rhs_itr) {
      ++lhs_itr;
    } else {
      if (!(*rhs_itr < *lhs_itr)) {
        lhs_itr = lhs.erase(lhs_itr);
        rhs_itr = rhs.erase(rhs_itr);
      } else {
        ++rhs_itr;
      }
    }
  }
}

}  // namespace



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



DiskBasedStorage::FileDetails::FileDetails(const protobuf::DiskStoredFile& file,
                                           const crypto::SHA1Hash& hash_in)
    : min_element(file.element(0).name().substr(0, FileDetails::kSubstrSize)),
      max_element(file.element(file.element_size() - 1).name().substr(0, FileDetails::kSubstrSize)),
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
  FileDetails file_details(file, hash);
  auto file_and_details(std::make_pair(file, file_details));
  if (verify)
    VerifyFile(file_and_details);

  return file_and_details;
}

void DiskBasedStorage::VerifyFile(
    std::pair<protobuf::DiskStoredFile, FileDetails>& file_and_details) const {
  // TODO(Fraser#5#): 2013-02-14 - Consider different errors here to indicate probable malicious
  //                               tampering with file contents.
  SortFile(file_and_details.first, true);
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
    if (next != std::end(file_ids_)) {
      if ((*next).second.min_element < (*itr).second.max_element ||
          (*next).first != (*itr).first + 1) {
        ThrowError(CommonErrors::parsing_error);
      }
    }
    ++itr;
  }
}

void DiskBasedStorage::ApplyRecentOperations(const std::vector<RecentOperation>& recent_ops) {
  SetCurrentOps(recent_ops);
  auto itr(GetReorganiseStartPoint());
  if (itr != std::end(file_ids_))
    MergeFromProtobufElements(ParseFile(*itr, false).first, current_puts_);

  while (itr != std::end(file_ids_)) {
    auto next(itr);
    if (next != std::end(file_ids_))
      MergeFromProtobufElements(ParseFile(*next, false).first, current_puts_);
    RemoveIntersection(current_puts_, current_deletes_);
    SaveFile(itr);
    ++itr;
  }
  if (!current_puts_.empty())
    SaveFile(itr);

  assert(current_puts_.empty());
  assert(current_deletes_.empty());
}

void DiskBasedStorage::SetCurrentOps(std::vector<RecentOperation> recent_ops) {
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

DiskBasedStorage::FileGroup::iterator DiskBasedStorage::GetFileIdsLowerBound(
    const FileDetails::ElementNameSubstr& element_name_substr) {
  return std::lower_bound(std::begin(file_ids_),
                          std::end(file_ids_),
                          element_name_substr,
                          [](const FileGroup::value_type& file_id,
                             const FileDetails::ElementNameSubstr& min_name) {
                            return file_id.second.max_element < min_name;
                          });
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
  auto lower(GetFileIdsLowerBound(min_element_name));
  return (lower == std::end(file_ids_) && !file_ids_.empty()) ? --lower : lower;
}

void DiskBasedStorage::SaveFile(FileGroup::iterator file_ids_itr) {
  auto file(MoveCurrentPutsToFile());
  NonEmptyString new_content(file.SerializeAsString());
  FileDetails new_details(file, crypto::Hash<crypto::SHA1>(new_content));

  bool is_replacement(file_ids_itr != std::end(file_ids_));
  int index(is_replacement ? (*file_ids_.rbegin()).first + 1 : (*file_ids_itr).first);
  fs::path new_path(kRoot_ / GetFileName(std::make_pair(index, new_details)));
  if (!WriteFile(new_path, new_content.string()))
    ThrowError(CommonErrors::filesystem_io_error);

  if (is_replacement) {
    fs::path old_path(kRoot_ / GetFileName(*file_ids_itr));
    boost::system::error_code ec;
    if (!boost::filesystem::remove(old_path, ec)) {
      LOG(kError) << "Failed to remove " << old_path << ":  " << ec.message();
      // Remove new file to achieve strong exception guarantee
      boost::filesystem::remove(new_path, ec);
      ThrowError(CommonErrors::filesystem_io_error);
    }
    (*file_ids_itr).second = new_details;
  } else {
    file_ids_.insert(std::end(file_ids_), std::make_pair(index, new_details));
  }
}

protobuf::DiskStoredFile DiskBasedStorage::MoveCurrentPutsToFile() {
  protobuf::DiskStoredFile file;
  GetTagValueAndIdentityVisitor type_and_name_visitor;
  int count(0);
  auto current_puts_itr(std::begin(current_puts_));
  while (count < detail::Parameters::max_file_element_count &&
         current_puts_itr != std::end(current_puts_)) {
    auto proto_element(file.add_element());
    auto type_and_name(boost::apply_visitor(type_and_name_visitor, (*current_puts_itr).first));
    proto_element->set_type(static_cast<int32_t>(type_and_name.first));
    proto_element->set_name(type_and_name.second.string());
    proto_element->set_value((*current_puts_itr).second);
    current_puts_itr = current_puts_.erase(current_puts_itr);
  }
  return file;
}

void DiskBasedStorage::ApplyAccountTransfer(const fs::path& transferred_files_dir) {
  std::vector<fs::path> files_moved, files_to_be_removed;
  on_scope_exit strong_guarantee([this, &files_moved] {
      on_scope_exit::RevertValue(file_ids_)();
      boost::system::error_code ec;
      for (const auto& file : files_moved)
        fs::remove(file, ec);
  });

  try {
    FileGroup transfer_ids(GetFilesToTransfer(transferred_files_dir));
    for (const auto& transfer_id : transfer_ids)
      TransferFile(transfer_id, transferred_files_dir, files_moved, files_to_be_removed);
    VerifyFileGroup();
    boost::system::error_code ec;
    for (const auto& filepath : files_to_be_removed)
      fs::remove(filepath, ec);
  }
  catch(const std::exception& e) {
    LOG(kWarning) << "Failed to apply account transfer: " << e.what();
  }

  strong_guarantee.Release();
}

DiskBasedStorage::FileGroup DiskBasedStorage::GetFilesToTransfer(
    const fs::path& transferred_files_dir) const {
  FileGroup transfer_ids;
  int current_max_index(file_ids_.empty() ? -1 : (*file_ids_.rbegin()).first);
  fs::directory_iterator dir_itr(transferred_files_dir), end_itr;
  std::vector<fs::path> exact_duplicates;
  for (; dir_itr != end_itr; ++dir_itr) {
    auto file_and_details(ParseFile(fs::path(*dir_itr), true));
    auto file_id(std::make_pair(std::stoi((*dir_itr).path().stem().string()),
                                file_and_details.second));
    if (file_id.first <= current_max_index) {
      auto itr(std::begin(file_ids_));
      std::advance(itr, file_id.first);
      assert((*itr).first == file_id.first);
      if ((*itr).second.hash == file_id.second.hash) {
        // Replacement file is identical to existing file - don't move the replacement over.
        exact_duplicates.push_back(*dir_itr);
        continue;
      }
    }
    if (!transfer_ids.insert(file_id).second)
      ThrowError(CommonErrors::parsing_error);
  }

  boost::system::error_code ec;
  for (const auto& file : exact_duplicates)
    fs::remove(file, ec);

  return transfer_ids;
}

void DiskBasedStorage::TransferFile(const FileGroup::value_type& transfer_id,
                                    const fs::path& transferred_files_dir,
                                    std::vector<fs::path>& files_moved,
                                    std::vector<fs::path>& files_to_be_removed) {
  auto filename(GetFileName(transfer_id));
  fs::rename(transferred_files_dir / filename, kRoot_ / filename);
  files_moved.push_back(kRoot_ / filename);
  auto existing_itr(file_ids_.find(transfer_id.first));
  if (existing_itr == std::end(file_ids_)) {
    file_ids_.insert(existing_itr, transfer_id);
  } else {
    files_to_be_removed.push_back(kRoot_ / GetFileName(*existing_itr));
    (*existing_itr).second = transfer_id.second;
  }
}

std::vector<fs::path> DiskBasedStorage::GetFilenames() const {
  std::vector<fs::path> filenames;
  filenames.reserve(file_ids_.size());
  for (const auto& file_id : file_ids_)
    filenames.emplace_back(GetFileName(file_id));
  return filenames;
}

NonEmptyString DiskBasedStorage::GetFile(const fs::path& filename) const {
  return ReadFile(kRoot_ / filename);
}

}  // namespace vault

}  // namespace maidsafe
