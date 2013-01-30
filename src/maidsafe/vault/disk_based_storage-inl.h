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

typedef std::promise<void> VoidPromise;
typedef std::shared_ptr<VoidPromise> VoidPromisePtr;

template<typename Data>
std::future<void> DiskBasedStorage::Store(const typename Data::name_type& name,
                                          const std::string& serialised_value) {
  VoidPromisePtr promise(std::make_shared<VoidPromise>());
  std::future<void> future(promise->get_future());
  active_.Send([name, serialised_value, promise, this] {
                 try {
                   protobuf::DiskStoredElement element;
                   element.set_data_name(name.data.string());
                   element.set_serialised_value(serialised_value);
                   this->AddToLatestFile(element);
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
  active_.Send([name, promise, this] {
                 try {
                   protobuf::DiskStoredElement element;
                   element.set_data_name(name.data.string());
                   this->SearchForAndDeleteEntry(element);
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
  active_.Send([name, functor, serialised_value, promise, this] () {
                 try {
                   protobuf::DiskStoredElement element;
                   element.set_data_name(name.data.string());
                   element.set_serialised_value(serialised_value);
                   this->SearchForAndModifyEntry(element, functor);
                   promise->set_value();
                 }
                 catch(const std::exception& e) {
                   LOG(kError) << "Execution of Modify threw: " << e.what();
                   promise->set_exception(std::current_exception());
                 }
               });
  return std::move(future);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DISK_BASED_STORAGE_INL_H_
