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


namespace maidsafe {

namespace vault {

template<typename Data>
size_t DiskBasedStorage::GetElementCount(const typename Data::name_type& name) const {
  FileDetails::ElementNameSubstr name_substr(name->string().substr(0, FileDetails::kSubstrSize));
  auto lower(GetFileIdsLowerBound(name_substr));
  if (lower == std::end(file_ids_))
    return 0;
  auto file(ParseFile(*lower, false).first);

template<typename Data>
void DiskBasedStorage::AddToLatestFile(const typename Data::name_type& name, int32_t value) {
  if (file_ids_.size() == 0) {
      std::pair<int, crypto::SHA512Hash> init_id;
    init_id.first = 0;
    init_id.second = std::move(crypto::SHA512Hash());
    file_ids_.insert(init_id);
  }
  std::pair<int, crypto::SHA512Hash> file_id(*file_ids_.rbegin());
  auto file(ParseFile(file_id));
  if (file.element_size() == detail::Parameters::max_file_element_count) {
    ++file_id.first;
    file_id.second = std::move(crypto::SHA512Hash());
    file_ids_.insert(file_id);
    file.Clear();
  }
  AddElement<Data>(name, value, file_id, file);
}

template<typename Data>
void DiskBasedStorage::AddElement(const typename Data::name_type& name,
                                  int32_t value,
                                  const FileIdentity& file_id,
                                  protobuf::DiskStoredFile& file) {
  auto proto_element(file.add_element());
  proto_element->set_type(static_cast<int32_t>(Data::type_enum_value()));
  proto_element->set_name(name->string());
  proto_element->set_value(value);
  assert(file.element_size() <= detail::Parameters::max_file_element_count);
  SaveChangedFile(file_id, file);
}

template<typename Data>
std::future<int32_t> DiskBasedStorage::Delete(const typename Data::name_type& name) {
  auto promise(std::make_shared<std::promise<int32_t>>());
  active_.Send([name, promise, this] {
                 try {
                   int32_t value(0);
                   this->FindAndDeleteEntry<Data>(name, value);
                   promise->set_value(value);
                 }
                 catch(const std::exception& e) {
                   LOG(kError) << "Execution of Delete threw: " << e.what();
                   promise->set_exception(std::current_exception());
                 }
               });
  return promise->get_future();
}

template<typename Data>
void DiskBasedStorage::FindAndDeleteEntry(const typename Data::name_type& name, int32_t& value) {
  auto ritr(file_ids_.rbegin());
  if (!(*ritr).second.IsInitialised())
    ++ritr;

  for (; ritr != file_ids_.rend(); ++ritr) {
    auto file(ParseFile(*ritr));
    int index(GetEntryIndex<Data>(name, file));
    if (index >= 0) {
      value = file.element(index).value();
      DeleteEntry(index, file);
      SaveChangedFile(*ritr, file);
      return;
    }
  }

  ThrowError(CommonErrors::no_such_element);
}

template<typename Data>
int DiskBasedStorage::GetEntryIndex(const typename Data::name_type& name,
                                    const protobuf::DiskStoredFile& file) const {
  for (int i(0); i != file.element_size(); ++i) {
    if (file.element(i).name() == name->string() &&
        file.element(i).type() == static_cast<int32_t>(Data::type_enum_value())) {
      ++instances;
    } else if (instances) {
      break;
    }
  }
  return instances;
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DISK_BASED_STORAGE_INL_H_
