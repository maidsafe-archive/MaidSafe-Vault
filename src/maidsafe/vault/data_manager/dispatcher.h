/* Copyright 2013 MaidSafe.net limited

This MaidSafe Software is licensed under the MaidSafe.net Commercial License, version 1.0 or later,
and The General Public License (GPL), version 3. By contributing code to this project You agree to
the terms laid out in the MaidSafe Contributor Agreement, version 1.0, found in the root directory
of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available at:

http://www.novinet.com/license

Unless required by applicable law or agreed to in writing, software distributed under the License is
distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.
*/

#ifndef MAIDSAFE_VAULT_DATA_MANAGER_DISPATCHER_H_
#define MAIDSAFE_VAULT_DATA_MANAGER_DISPATCHER_H_

#include "maidsafe/common/error.h"
#include "maidsafe/common/types.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/data_types/data_type_values.h"
#include "maidsafe/routing/message.h"
#include "maidsafe/routing/routing_api.h"

#include "maidsafe/nfs/message_types.h"
#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/types.h"

namespace maidsafe {

namespace vault {

class DataManagerDispatcher {
 public:
  DataManagerDispatcher(routing::Routing& routing, const passport::Pmid& signing_fob);

  // To PmidManager
  template<typename Data>
  void SendGetRequest(const PmidName& pmid_node,
                      const typename Data::Name& data_name,
                      nfs::MessageId message_id);
  // To MaidNode
  template<typename Data>
  void SendGetResponse(const MaidName& maid_node,
                       const Data& data,
                       nfs::MessageId message_id);
  // To MaidNode (failure)
  template<typename Data>
  void SendGetResponse(const MaidName& maid_node,
                       const typename Data::Name& data_name,
                       const maidsafe_error& result,
                       nfs::MessageId message_id);

  // To PmidNode
  template<typename Data>
  void SendGetResponse(const PmidName& pmid_node,
                       const Data& data,
                       nfs::MessageId message_id);

  // To PmidManager
  template<typename Data>
  void SendPutRequest(const PmidName& pmid_name,
                      const Data& data,
                      nfs::MessageId message_id);

  // To MaidManager
  template<typename Data>
  void SendPutResponse(const MaidName& account_name,
                       const typename Data::Name& data_name,
                       const maidsafe_error& result,
                       const int32_t cost,
                       nfs::MessageId message_id);

  // To PmidManager
  template<typename Data>
  void SendDeleteRequest(const PmidName& pmid_name,
                         const typename Data::Name& data_name,
                         nfs::MessageId message_id);


  void SendSync(const NodeId& destination_peer,
                const MaidName& account_name,
                const std::string& serialised_sync);

  void SendAccountTransfer(const NodeId& destination_peer,
                           const MaidName& account_name,
                           const std::string& serialised_account);

 private:
  DataManagerDispatcher();
  DataManagerDispatcher(const DataManagerDispatcher&);
  DataManagerDispatcher(DataManagerDispatcher&&);
  DataManagerDispatcher& operator=(DataManagerDispatcher);

  template<typename Data>
  routing::GroupSource Sender(const typename Data::Name& data_name) const;

  routing::Routing& routing_;
  const passport::Pmid kSigningFob_;
  const routing::SingleId kThisNodeAsSender_;
};



// ==================== Implementation =============================================================

// To PmidManager
template<typename Data>
void DataManagerDispatcher::SendGetRequest(const PmidName& pmid_node,
                                           const typename Data::Name& data_name,
                                           nfs::MessageId message_id) {
  typedef GetRequestFromDataManagerToPmidNode NfsMessage;
  CheckSourcePersonaType<NfsMessage>();
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
  static const routing::Cacheable kCacheable(is_cacheable<Data>::value ? routing::Cacheable::kGet :
                                                                         routing::Cacheable::kNone);
  NfsMessage nfs_message((message_id, NfsMessage::Contents(data_name)));
  NfsMessage::Receiver receiver(routing::SingleId(NodeId(pmid_node->string())));
  routing_.Send(RoutingMessage(nfs_message.Serialise(), Sender(data.name), receiver, kCacheable));
}

// To MaidNode
template<typename Data>
void SendGetResponse(const MaidName& maid_node,
                     const Data& data,
                     nfs::MessageId message_id) {
  typedef SendGetResponseFromDataManagerToMaidNode NfsMessage;
  CheckSourcePersonaType<NfsMessage>();
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
  static const routing::Cacheable kCacheable(is_cacheable<Data>::value ? routing::Cacheable::kPut :
                                                                         routing::Cacheable::kNone);
  NfsMessage::Contents contents;
  *contents.data = data;
  NfsMessage nfs_message((message_id, contents));
  NfsMessage::Receiver receiver(routing::SingleId(NodeId(maid_node->string())));
  routing_.Send(RoutingMessage(nfs_message.Serialise(), Sender(data.name), receiver, kCacheable));
}


template<typename Data>
void DataManagerDispatcher::SendPutRequest(const PmidName& pmid_name,
                                           const Data& data,
                                           nfs::MessageId message_id) {
  typedef PutRequestFromDataManagerToPmidManager NfsMessage;
  CheckSourcePersonaType<NfsMessage>();
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
  static const routing::Cacheable kCacheable(is_cacheable<Data>::value ? routing::Cacheable::kPut :
                                                                         routing::Cacheable::kNone);
  NfsMessage nfs_message((message_id, NfsMessage::Contents(data)));
  routing::GroupId receiver(NodeId(pmid_name->string()));
  routing_.Send(RoutingMessage(nfs_message.Serialise(), Sender(data.name), receiver, kCacheable));
}


template<typename Data>
routing::GroupSource MaidManagerDispatcher::Sender(const typename Data::Name& data_name) const {
  return routing::GroupSource(routing::GroupId(NodeId(data_name->string())),
                              routing::SingleId(routing_.kNodeId()));
}

template<typename Message>
void MaidNodeDispatcher::CheckSourcePersonaType() const {
  static_assert(NfsMessage::SourcePersona::value == Persona::kDataManager,
                  "The source Persona must be kDataManager.");
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_DISPATCHER_H_
