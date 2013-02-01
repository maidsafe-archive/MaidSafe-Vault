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

#ifndef MAIDSAFE_VAULT_DISK_BASED_STORAGE_INL_H_
#define MAIDSAFE_VAULT_DISK_BASED_STORAGE_INL_H_

#include <string>

#include "boost/filesystem/operations.hpp"

#include "maidsafe/common/utils.h"

#include "maidsafe/data_types/data_name_variant.h"

#include "maidsafe/vault/disk_based_storage_messages_pb.h"
#include "maidsafe/vault/parameters.h"

namespace maidsafe {

namespace vault {

typedef std::promise<void> VoidPromise;
typedef std::shared_ptr<VoidPromise> VoidPromisePtr;

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

template<typename Data>
struct DiskBasedStorage::StoredElement {
   StoredElement() : name(), serialised_value() {}
   StoredElement(const typename Data::name_type& the_name, const std::string& the_serialised_value)
       : name(the_name),
         serialised_value(the_serialised_value) {}
   typename Data::name_type name;
   std::string serialised_value;
};

template<typename Data>
std::future<void> DiskBasedStorage::Store(const typename Data::name_type& name,
                                          const std::string& serialised_value) {
  VoidPromisePtr promise(std::make_shared<VoidPromise>());
  std::future<void> future(promise->get_future());
  StoredElement<Data> element(name, serialised_value);
  active_.Send([element, promise, this] {
                 try {
                   this->AddToLatestFile<Data>(element);
                   promise->set_value();
                 }
                 catch(const std::exception& e) {
                   LOG(kError) << "Execution of Store threw: " << e.what();
                   promise->set_exception(std::current_exception());
                 }
               });
  return std::move(future);
}

template<typename Data>
std::future<void> DiskBasedStorage::Delete(const typename Data::name_type& name) {
  VoidPromisePtr promise(std::make_shared<VoidPromise>());
  std::future<void> future(promise->get_future());
  StoredElement<Data> element(name, "");
  active_.Send([element, promise, this] {
                 try {
                   this->SearchForAndDeleteEntry<Data>(element);
                   promise->set_value();
                 }
                 catch(const std::exception& e) {
                   LOG(kError) << "Execution of Delete threw: " << e.what();
                   promise->set_exception(std::current_exception());
                 }
               });
  return std::move(future);
}

template<typename Data>
std::future<void> DiskBasedStorage::Modify(const typename Data::name_type& name,
                                           const std::function<void(std::string&)>& functor,
                                           const std::string& serialised_value) {
  assert(functor && "Null functor not allowed!");
  VoidPromisePtr promise(std::make_shared<VoidPromise>());
  std::future<void> future(promise->get_future());
  StoredElement<Data> element(name, serialised_value);
  active_.Send([element, functor, promise, this] () {
                 try {
                   this->SearchForAndModifyEntry<Data>(element, functor);
                   promise->set_value();
                 }
                 catch(const std::exception& e) {
                   LOG(kError) << "Execution of Modify threw: " << e.what();
                   promise->set_exception(std::current_exception());
                 }
               });
  return std::move(future);
}

template<typename Data>
void DiskBasedStorage::AddToLatestFile(const DiskBasedStorage::StoredElement<Data>& element) {
  size_t latest_file_index(file_hashes_.size() - 1);
  protobuf::DiskStoredFile disk_file;
  boost::filesystem::path latest_file_path;
  NonEmptyString file_content;
  if (file_hashes_.at(latest_file_index) != kEmptyFileHash_) {
    ReadAndParseFile(file_hashes_.at(latest_file_index),
                     latest_file_index,
                     disk_file,
                     latest_file_path,
                     file_content);
  }

  AddDiskElementToFileDisk(element, disk_file);
  NonEmptyString new_content(disk_file.SerializeAsString());
  if (disk_file.disk_element_size() == int(detail::Parameters::max_file_element_count())) {
    // Increment the file
    file_hashes_.push_back(kEmptyFileHash_);
  }

  boost::filesystem::path new_path(GetFilePath(kRoot_,
                                               file_hashes_.at(latest_file_index),
                                               latest_file_index));
  WriteFile(new_path, new_content.string());
  if (!latest_file_path.empty() && latest_file_path != new_path) {
    try {
      boost::filesystem::remove(latest_file_path);
    }
    catch(const std::exception& e) {
      LOG(kError) << "Failed to remove old file. Will erase new file to leave state as it was.";
      boost::filesystem::remove(new_path);
      ThrowError(VaultErrors::failed_to_handle_request);
    }
    file_hashes_.at(latest_file_index) = crypto::Hash<crypto::SHA512>(new_content);
  }
}

template<typename Data>
void DiskBasedStorage::SearchForAndDeleteEntry(
    const DiskBasedStorage::StoredElement<Data>& element) {
  Changer changer;
  SearchForEntryAndExecuteOperation(element, changer, true);
}

template<typename Data>
void DiskBasedStorage::SearchForAndModifyEntry(const DiskBasedStorage::StoredElement<Data>& element,
                                               const std::function<void(std::string&)>& functor) {
  Changer changer(functor);
  SearchForEntryAndExecuteOperation(element, changer, false);
}

template<typename Data>
void DiskBasedStorage::SearchForEntryAndExecuteOperation(
    const DiskBasedStorage::StoredElement<Data>& element,
    Changer& changer,
    bool reorder) {
  size_t file_index(file_hashes_.size() - 1);
  boost::filesystem::path file_path;
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

template<typename Data>
void DiskBasedStorage::AddDiskElementToFileDisk(
    const DiskBasedStorage::StoredElement<Data>& element,
    protobuf::DiskStoredFile& disk_file) {
  protobuf::DiskStoredElement* disk_element(disk_file.add_disk_element());
  disk_element->set_data_name(element.name.data.string());
  disk_element->set_serialised_value(element.serialised_value);
  disk_element->set_data_type(static_cast<int32_t>(Data::name_type::tag_type::kEnumValue));
}

// This function expects lhs to have data name and value always
template<typename Data>
bool DiskBasedStorage::MatchingDiskElements(
    const protobuf::DiskStoredElement& lhs,
    const DiskBasedStorage::StoredElement<Data>& element) const {
  return lhs.data_name() == element.name.data.string() &&
         lhs.data_type() == static_cast<int32_t>(Data::name_type::tag_type::kEnumValue) &&
         (element.serialised_value.empty() ?
              true :
              lhs.serialised_value() == element.serialised_value);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DISK_BASED_STORAGE_INL_H_
