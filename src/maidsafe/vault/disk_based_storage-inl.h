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

#include <algorithm>
#include <string>
#include <vector>

#include "maidsafe/common/utils.h"
#include "maidsafe/common/on_scope_exit.h"

#include "maidsafe/vault/parameters.h"


namespace maidsafe {

namespace vault {

namespace detail {

void SortElements(std::vector<protobuf::DiskStoredElement>& elements);

void SortFile(protobuf::DiskStoredFile& file);

}  // namespace detail

template<typename Data>
std::future<void> DiskBasedStorage::Store(const typename Data::name_type& name, int32_t value) {
  auto promise(std::make_shared<std::promise<void>>());
  active_.Send([name, value, promise, this] {
                 try {
                   this->AddToLatestFile<Data>(name, value);
                   promise->set_value();
                 }
                 catch(const std::exception& e) {
                   LOG(kError) << "Execution of Store threw: " << e.what();
                   promise->set_exception(std::current_exception());
                 }
               });
  return promise->get_future();
}

template<typename Data>
void DiskBasedStorage::AddToLatestFile(const typename Data::name_type& name, int32_t value) {
  FileIdentity file_id(*file_ids_.rbegin());
  if (file_id.second.IsInitialised()) {  // The file already exists: add to this file.
    auto file(ParseFile(file_id));
    AddElement<Data>(name, value, file_id, file);
  } else {  // The file doesn't already exist - create a new file.
    protobuf::DiskStoredFile file;
    AddElement<Data>(name, value, file_id, file);
  }
}

template<typename Data>
void DiskBasedStorage::AddElement(const typename Data::name_type& name,
                                  int32_t value,
                                  const FileIdentity& file_id,
                                  protobuf::DiskStoredFile& file) {
  on_scope_exit disk_stored_file_guarantee(on_scope_exit::RevertValue(file));

  auto proto_element(file.add_element());
  proto_element->set_type(static_cast<int32_t>(Data::name_type::tag_type::kEnumValue));
  proto_element->set_name(name->string());
  proto_element->set_value(value);
  assert(file.element_size() <= detail::Parameters::max_file_element_count());

  if (file.element_size() == detail::Parameters::max_file_element_count()) {
    // Insert a placeholder with a default-initialised (invalid) hash to indicate we need a new
    // file at the next AddToLatestFile.
    file_ids_.insert(std::make_pair(file_id.first + 1, crypto::SHA512Hash()));
    detail::SortFile(file);
  }

  SaveChangedFile(file_id, file);
  disk_stored_file_guarantee.Release();
}

template<typename Data>
std::future<int32_t> DiskBasedStorage::Delete(const typename Data::name_type& name) {
  auto promise(std::make_shared<std::promise<int32_t>>());
  active_.Send([name, promise, this] {
                 try {
                   int32_t value(0);
                   bool reorganise_files(false);
                   this->FindAndDeleteEntry<Data>(name, value, reorganise_files);
                   promise->set_value(value);
                   if (reorganise_files)
                     this->ReorganiseFiles();
                 }
                 catch(const std::exception& e) {
                   LOG(kError) << "Execution of Delete threw: " << e.what();
                   promise->set_exception(std::current_exception());
                 }
               });
  return promise->get_future();
}

template<typename Data>
void DiskBasedStorage::FindAndDeleteEntry(const typename Data::name_type& name,
                                          int32_t& value,
                                          bool& reorganise_files) {
  auto ritr(file_ids_.rbegin());
  if (!(*ritr).second.IsInitialised())
    ++ritr;

  for (; ritr != file_ids_.rend(); ++ritr) {
    auto file(ParseFile(*ritr));
    int index(GetEntryIndex(name, file));
    if (index >= 0) {
      value = file.element(index).value();
      DeleteEntry(index, file);
      SaveChangedFile(*ritr, file);
      if ((*ritr).first < oldest_non_full_file_index_)
        oldest_non_full_file_index_ = (*ritr).first;
      reorganise_files = (file.element_size() < detail::Parameters::min_file_element_count() &&
                          ritr != file_ids_.rbegin());
      return;
    }
  }

  ThrowError(CommonErrors::no_such_element);
}

template<typename Data>
int DiskBasedStorage::GetEntryIndex(const typename Data::name_type& name,
                                    const protobuf::DiskStoredFile& file) const {
  for (int i(0); i != file.element_size(); ++i) {
    if (file.element(i).type() == static_cast<int32_t>(Data::name_type::tag_type::kEnumValue) &&
        file.element(i).name() == name->string()) {
      return i;
    }
  }
  return -1;
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DISK_BASED_STORAGE_INL_H_
