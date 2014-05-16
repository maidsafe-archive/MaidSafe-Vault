/*  Copyright 2012 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.maidsafe.net/licenses

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#include "maidsafe/vault/utils.h"

#include <string>

#include "boost/filesystem/operations.hpp"
#include "leveldb/status.h"

#include "maidsafe/common/types.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/vault/parameters.h"
#include "maidsafe/vault/pmid_node/service.h"

namespace fs = boost::filesystem;

namespace maidsafe {

namespace vault {

namespace detail {

template <typename T>
DataNameVariant GetNameVariant(const T&) {
  T::invalid_parameter;
  return DataNameVariant();
}

template <>
DataNameVariant GetNameVariant(const nfs_vault::DataName& data) {
  return GetDataNameVariant(data.type, data.raw_name);
}

template <>
DataNameVariant GetNameVariant(const nfs_vault::DataNameAndContent& data) {
  return GetNameVariant(data.name);
}

template <>
DataNameVariant GetNameVariant(const nfs_vault::DataNameAndContentOrCheckResult& data) {
  return GetNameVariant(data.name);
}

template <>
DataNameVariant GetNameVariant(const nfs_vault::DataNameAndCost& data) {
  return GetNameVariant(data.name);
}

template <>
DataNameVariant GetNameVariant(const nfs_vault::DataNameAndSize& data) {
  return GetNameVariant(data.name);
}

template <>
DataNameVariant GetNameVariant(const nfs_vault::DataAndPmidHint& data) {
  return GetNameVariant(data.data.name);
}

template <>
DataNameVariant GetNameVariant(const nfs_client::DataAndReturnCode& data) {
  return GetNameVariant(data.data.name);
}

template <>
DataNameVariant GetNameVariant(const nfs_client::DataNameAndContentOrReturnCode& data) {
  return GetNameVariant(data.name);
}

template <>
DataNameVariant GetNameVariant(const nfs_client::DataNameAndReturnCode& data) {
  return GetNameVariant(data.name);
}

template <>
DataNameVariant GetNameVariant(const nfs_client::DataNameAndSpaceAndReturnCode& data) {
  return GetNameVariant(data.name);
}

template <>
DataNameVariant GetNameVariant(const nfs_vault::DataNameAndVersion& data) {
  return GetNameVariant(data.data_name);
}

template <>
DataNameVariant GetNameVariant(const nfs_vault::DataNameOldNewVersion& data) {
  return GetNameVariant(data.data_name);
}

template <>
DataNameVariant GetNameVariant(const nfs_vault::DataNameAndRandomString& data) {
  return GetNameVariant(data.name);
}

template <>
DataNameVariant GetNameVariant(const nfs_vault::VersionTreeCreation& data) {
  return GetNameVariant(data.data_name);
}

void InitialiseDirectory(const boost::filesystem::path& directory) {
  if (fs::exists(directory)) {
    if (!fs::is_directory(directory))
      BOOST_THROW_EXCEPTION(MakeError(CommonErrors::not_a_directory));
  } else {
    fs::create_directory(directory);
  }
}

bool ShouldRetry(routing::Routing& routing, const NodeId& source_id, const NodeId& data_name) {
  return routing.network_status() >= Parameters::kMinNetworkHealth &&
         routing.EstimateInGroup(source_id, data_name);
}

}  // namespace detail

boost::filesystem::path UniqueDbPath(const boost::filesystem::path& vault_root_dir) {
  boost::filesystem::path db_root_path(vault_root_dir / "db");
  detail::InitialiseDirectory(db_root_path);
  return (db_root_path / boost::filesystem::unique_path());
}

std::unique_ptr<leveldb::DB> InitialiseLevelDb(const boost::filesystem::path& db_path) {
  if (boost::filesystem::exists(db_path))
    boost::filesystem::remove_all(db_path);
  leveldb::DB* db(nullptr);
  leveldb::Options options;
  options.create_if_missing = true;
  options.error_if_exists = true;
  leveldb::Status status(leveldb::DB::Open(options, db_path.string(), &db));
  if (!status.ok())
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::filesystem_io_error));
  assert(db);
  return std::move(std::unique_ptr<leveldb::DB>(db));
}

nfs::MessageId HashStringToMessageId(const std::string& input) {
  std::hash<std::string> hash_fn;
  return nfs::MessageId(static_cast<nfs::MessageId::value_type>(hash_fn(input)));
}

}  // namespace vault

}  // namespace maidsafe
