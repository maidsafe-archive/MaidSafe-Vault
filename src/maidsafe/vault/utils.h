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
DataNameVariant GetNameVariant(const nfs_vault::DataNameAndContentOrCheckResult& data);

template <>
DataNameVariant GetNameVariant(const nfs_vault::DataNameAndCost& data);

template <>
DataNameVariant GetNameVariant(const nfs_vault::DataNameAndSize& data);

template <>
DataNameVariant GetNameVariant(const nfs_vault::DataAndPmidHint& data);

template <>
DataNameVariant GetNameVariant(const nfs_client::DataAndReturnCode& data);

template <>
DataNameVariant GetNameVariant(const nfs_client::DataNameAndContentOrReturnCode& data);

template <>
DataNameVariant GetNameVariant(const nfs_client::DataNameAndReturnCode& data);

template <>
DataNameVariant GetNameVariant(const nfs_client::DataNameAndSpaceAndReturnCode& data);

template <>
DataNameVariant GetNameVariant(const nfs_vault::DataNameAndVersion& data);

template <>
DataNameVariant GetNameVariant(const nfs_vault::DataNameOldNewVersion& data);

template <>
DataNameVariant GetNameVariant(const nfs_vault::DataNameAndRandomString& data);

template <typename MessageType>
struct ValidateSenderType {
  typedef std::function<bool(const MessageType&, const typename MessageType::Sender&)> type;
};

// =============================================================================================

template <typename RequestorPersona>
struct Requestor {
  typedef RequestorPersona SourcePersonaType;
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

template <typename ServiceHandlerType, typename MessageType>
void DoOperation(ServiceHandlerType* service, const MessageType& message,
                 const typename MessageType::Sender& sender,
                 const typename MessageType::Receiver& receiver);

// The following 3 structs are all helpers relating to SendSyncMessage below.
template <typename T>
struct ToVoid { typedef void type; };

// This struct applies to personas which don't have a public type 'GroupName'.
template <typename PersonaType, typename Enable = void>
struct GroupOrKeyType {
  typedef typename PersonaType::Key Type;

  static std::string ToString(const Type& key) { return key.name.string(); }

  static routing::GroupSource GroupSender(const routing::Routing& routing, const Type& key) {
    return routing::GroupSource(routing::GroupId(NodeId(ToString(key))),
                                routing::SingleId(routing.kNodeId()));
  }
};

// This struct applies to personas which do have a public type 'GroupName'.
template <typename PersonaType>
struct GroupOrKeyType<PersonaType, typename ToVoid<typename PersonaType::GroupName>::type> {
  typedef typename PersonaType::GroupName Type;

  static std::string ToString(const Type& group_name) { return group_name->string(); }

  static routing::GroupSource GroupSender(const routing::Routing& routing, const Type& group_name) {
    return routing::GroupSource(routing::GroupId(NodeId(ToString(group_name))),
                                routing::SingleId(routing.kNodeId()));
  }
};

}  // namespace detail

std::unique_ptr<leveldb::DB> InitialiseLevelDb(const boost::filesystem::path& db_path);

// ============================ dispatcher utils ===================================================
template <typename MessageType>
struct SendSyncMessage {
  typedef routing::Message<typename MessageType::Sender,
                           typename MessageType::Receiver> RoutingMessage;
  typedef nfs::PersonaTypes<MessageType::SourcePersona::value> PersonaType;
  typedef detail::GroupOrKeyType<PersonaType> GroupOrKeyHelper;

  void operator()(routing::Routing& routing, const MessageType& sync_message,
                  const typename detail::GroupOrKeyType<PersonaType>::Type& group_id) {
    static_assert(MessageType::SourcePersona::value == MessageType::DestinationPersona::value,
                  "Sync messages must be sent to same persona.");
    static_assert(std::is_same<typename MessageType::Sender, routing::GroupSource>::value,
                  "Sync message Senders must be GroupSource type.");
    static_assert(std::is_same<typename MessageType::Receiver, routing::GroupId>::value,
                  "Sync message Receivers must be GroupId type.");

    RoutingMessage message(sync_message.Serialise(),
                           GroupOrKeyHelper::GroupSender(routing, group_id),
                           routing::GroupId(NodeId(GroupOrKeyHelper::ToString(group_id))));
    routing.Send(message);
  }
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/utils-inl.h"

#endif  // MAIDSAFE_VAULT_UTILS_H_
