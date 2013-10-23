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

#ifndef MAIDSAFE_VAULT_UTILS_H_
#define MAIDSAFE_VAULT_UTILS_H_

#include "leveldb/db.h"

#include "maidsafe/common/node_id.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/routing/routing_api.h"

#include "maidsafe/nfs/message_types.h"
#include "maidsafe/vault/key_utils.h"


namespace maidsafe {

namespace vault {

namespace detail {

template <typename T>
DataNameVariant GetNameVariant(const T&);

template <>
DataNameVariant GetNameVariant(const nfs_vault::DataName& data);

template <>
DataNameVariant GetNameVariant(const nfs_vault::DataNameAndContent& data);

template <>
DataNameVariant GetNameVariant(const nfs_vault::DataAndPmidHint& data);

template <>
DataNameVariant GetNameVariant(const nfs_client::DataAndReturnCode& data);

template <>
DataNameVariant GetNameVariant(const nfs_client::DataNameAndContentOrReturnCode& data);

template <>
DataNameVariant GetNameVariant(const nfs_vault::DataNameAndContentOrCheckResult& data);

template <typename MessageType>
struct ValidateSenderType {
  typedef std::function<bool(const MessageType&, const typename MessageType::Sender&)> type;
};

// =============================================================================================

template <typename RequestorPersona>
struct Requestor {
  Requestor();
  explicit Requestor(NodeId node_id_in) : node_id(std::move(node_id_in)) {}
  Requestor(const Requestor& other) : node_id(other.node_id) {}
  Requestor(Requestor&& other) : node_id(std::move(other.node_id)) {}
  friend void swap(Requestor& lhs, Requestor& rhs) MAIDSAFE_NOEXCEPT {
    using std::swap;
    swap(lhs.node_id, rhs.node_id);
  }
  Requestor& operator=(Requestor other) {
    swap(*this, other);
    return *this;
  }

  NodeId node_id;
  static const nfs::Persona persona_value = RequestorPersona::value;
};

void InitialiseDirectory(const boost::filesystem::path& directory);
// bool ShouldRetry(routing::Routing& routing, const nfs::Message& message);

template <typename Data>
bool IsDataElement(const typename Data::Name& name, const DataNameVariant& data_name_variant);

template <typename AccountSet, typename Account>
typename Account::serialised_type GetSerialisedAccount(std::mutex& mutex,
                                                       const AccountSet& accounts,
                                                       const typename Account::Name& account_name);

template <typename AccountSet, typename Account>
typename Account::serialised_info_type GetSerialisedAccountSyncInfo(
    std::mutex& mutex, const AccountSet& accounts, const typename Account::Name& account_name);

}  // namespace detail

std::unique_ptr<leveldb::DB> InitialiseLevelDb(const boost::filesystem::path& db_path);

// ============================ dispatcher utils ===================================================
template<typename Persona, typename MessageType>
typename std::enable_if<std::is_same<typename MessageType::Sender, routing::GroupSource>::value,
  typename MessageType::Sender>::type
GroupSender(const routing::Routing& routing, const typename Persona::GroupName& group_name) {
  return typename MessageType::Sender(routing::GroupId(NodeId(group_name.value.string())),
                                      routing::SingleId(routing.kNodeId()));
}

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/utils-inl.h"

#endif  // MAIDSAFE_VAULT_UTILS_H_
