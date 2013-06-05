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

#ifndef MAIDSAFE_VAULT_MANAGER_DB_INL_H_
#define MAIDSAFE_VAULT_MANAGER_DB_INL_H_

#include "maidsafe/vault/manager_db.h"

#include <iomanip>
#include <sstream>

#include "boost/filesystem/operations.hpp"

#include "maidsafe/common/log.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/data_types/data_type_values.h"
#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

template<typename PersonaType>
const int ManagerDb<PersonaType>::kSuffixWidth_(1);

template<>
const int ManagerDb<StructuredDataManager>::kSuffixWidth_(1);

template<typename PersonaType>
ManagerDb<PersonaType>::ManagerDb(const boost::filesystem::path& path)
    : kDbPath_(path),
      mutex_(),
      leveldb_() {
  if (boost::filesystem::exists(kDbPath_))
    boost::filesystem::remove_all(kDbPath_);
  leveldb::DB* db;
  leveldb::Options options;
  options.create_if_missing = true;
  options.error_if_exists = true;
  leveldb::Status status(leveldb::DB::Open(options, kDbPath_.string(), &db));
  if (!status.ok())
    ThrowError(VaultErrors::failed_to_handle_request); // FIXME need new exception
  leveldb_ = std::move(std::unique_ptr<leveldb::DB>(db));
  assert(leveldb_);
}

template<typename PersonaType>
ManagerDb<PersonaType>::~ManagerDb() {
  leveldb::DestroyDB(kDbPath_.string(), leveldb::Options());
}

template<typename PersonaType>
void ManagerDb<PersonaType>::Put(const KvPair& key_value_pair) {

  leveldb::Status status(leveldb_->Put(leveldb::WriteOptions(),
                                       SerialiseKey(key_value_pair.first),
                                       key_value_pair.second.Serialise()->string()));
  if (!status.ok())
    ThrowError(VaultErrors::failed_to_handle_request);
}

template<typename PersonaType>
void ManagerDb<PersonaType>::Delete(const typename PersonaType::DbKey& key) {
  leveldb::Status status(leveldb_->Delete(leveldb::WriteOptions(), SerialiseKey(key)));
  if (!status.ok())
    ThrowError(VaultErrors::failed_to_handle_request);
}

template<typename PersonaType>
typename PersonaType::DbValue ManagerDb<PersonaType>::Get(const typename PersonaType::DbKey& key) {
  leveldb::ReadOptions read_options;
  read_options.verify_checksums = true;
  std::string value;
  leveldb::Status status(leveldb_->Get(read_options, SerialiseKey(key), &value));
  if (!status.ok())
    ThrowError(VaultErrors::failed_to_handle_request);
  assert(!value.empty());
  return typename PersonaType::DbValue(typename PersonaType::DbValue::serialised_type(
                                         NonEmptyString(value)));
}


// TODO(Team) This can be optimise by returning iterators.
template<typename PersonaType>
std::vector<typename PersonaType::DbKey> ManagerDb<PersonaType>::GetKeys() {
  std::vector<StructuredDataManager::DbKey> return_vector;
  std::lock_guard<std::mutex> lock(mutex_);
  std::unique_ptr<leveldb::Iterator> iter(leveldb_->NewIterator(leveldb::ReadOptions()));
  for (iter->SeekToFirst(); iter->Valid(); iter->Next())
    return_vector.push_back(ParseKey(iter->key().ToString()));
  return return_vector;
}

template<typename PersonaType>
std::string ManagerDb<PersonaType>::SerialiseKey(const typename PersonaType::DbKey& key) const {
  static GetTagValueAndIdentityVisitor visitor;
  auto result(boost::apply_visitor(visitor, key));
  return std::string(result.second.string() +
                     detail::ToFixedWidthString<kSuffixWidth_>(static_cast<uint32_t>(result.first)));
}

template<typename PersonaType>
typename PersonaType::DbKey ManagerDb<PersonaType>::ParseKey(
    const std::string& serialised_key) const {
  std::string name(serialised_key.substr(0, NodeId::kSize));
  std::string type_as_string(serialised_key.substr(NodeId::kSize, kSuffixWidth_));
  auto type(static_cast<DataTagValue>(detail::FromFixedWidthString<kSuffixWidth_>(type_as_string)));
  return GetDataNameVariant(type, Identity(name));
}

// Workaround for gcc 4.6 bug related to warning "redundant redeclaration" for template
// specialisation. refer // http://gcc.gnu.org/bugzilla/show_bug.cgi?id=15867#c4
#ifdef __GNUC__
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wredundant-decls"
#endif

template<>
std::string ManagerDb<StructuredDataManager>::SerialiseKey(
    const typename StructuredDataManager::DbKey& key) const;

template<>
typename StructuredDataManager::DbKey ManagerDb<StructuredDataManager>::ParseKey(
    const std::string& serialised_key) const;

#ifdef __GNUC__
#  pragma GCC diagnostic pop
#endif

}  // namespace vault

}  // namespace maidsafe

#endif // MAIDSAFE_VAULT_MANAGER_DB_INL_H_
