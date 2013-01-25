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

#include "maidsafe/vault/disk_based_storage_messages_pb.h"


namespace maidsafe {

namespace vault {

template<typename Data>
void DiskBasedStorage::Store(const typename Data::name_type& name,
                             int32_t version,
                             const std::string& serialised_value) {
  active_.Send([name, version, serialised_value, this] {
                   protobuf::DiskStoredElement element;
                   element.set_data_name(name.data.string());
                   element.set_version(version);
                   element.set_serialised_value(serialised_value);
                   this->AddToLatestFile(element);
               });
}

template<typename Data>
void DiskBasedStorage::Delete(const typename Data::name_type& name, int32_t version) {
  active_.Send([name, version, this] {
                   protobuf::DiskStoredElement element;
                   element.set_data_name(name.data.string());
                   element.set_version(version);
                   this->SearchForAndDeleteEntry(element);
               });
}

template<typename Data>
void DiskBasedStorage::Modify(const typename Data::name_type& name,
                              int32_t version,
                              const std::function<void(std::string&)>& functor,
                              const std::string& serialised_value) {
  assert(functor && "Null functor not allowed!");
  active_.Send([name, version, functor, serialised_value, this] () {
                   protobuf::DiskStoredElement element;
                   element.set_data_name(name.data.string());
                   element.set_version(version);
                   element.set_serialised_value(serialised_value);
                   this->SearchForAndModifyEntry(element, functor);
               });
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DISK_BASED_STORAGE_INL_H_
