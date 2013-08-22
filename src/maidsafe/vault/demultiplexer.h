/* Copyright 2012 MaidSafe.net limited

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

#ifndef MAIDSAFE_VAULT_DEMULTIPLEXER_H_
#define MAIDSAFE_VAULT_DEMULTIPLEXER_H_

#include <string>
#include <type_traits>

#include "maidsafe/common/log.h"
#include "maidsafe/common/types.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/message_wrapper.h"
#include "maidsafe/nfs/service.h"

#include "maidsafe/vault/data_manager/service.h"
#include "maidsafe/vault/maid_manager/service.h"
#include "maidsafe/vault/pmid_manager/service.h"
#include "maidsafe/vault/pmid_node/service.h"
#include "maidsafe/vault/version_manager/service.h"

namespace maidsafe {

namespace vault {

class Demultiplexer {
 public:
  Demultiplexer(MaidManagerService& maid_manager_service,
                VersionManagerService& version_manager_service,
                DataManagerService& data_manager_service,
                PmidManagerService& pmid_manager_service,
                PmidNodeService& pmid_node_service);
  template<typename T>
  void HandleMessage(const T& routing_message);
  template<typename T>
  bool GetFromCache(T& serialised_message);
  template<typename T>
  void StoreInCache(const T& serialised_message);

 private:
//  template<typename MessageType>
//  NonEmptyString HandleGetFromCache(const nfs::Message& message);
//  void HandleStoreInCache(const nfs::Message& message);

  nfs::Service<MaidManagerService>& maid_manager_service_;
  nfs::Service<VersionManagerService>& version_manager_service_;
  nfs::Service<DataManagerService>& data_manager_service_;
  nfs::Service<PmidManagerService>& pmid_manager_service_;
  nfs::Service<PmidNodeService>& pmid_node_service_;
};

template<typename T>
void Demultiplexer::HandleMessage(const T& routing_message) {
  auto wrapper_tuple(nfs::ParseMessageWrapper(routing_message.contents));
  const auto& destination_persona(std::get<2>(wrapper_tuple));
  static_assert(std::is_same<decltype(destination_persona),
                             const detail::DestinationTaggedValue&>::value,
                "The value retrieved from the tuple isn't the destination type, but should be.");
  switch (destination_persona.data) {
    case nfs::Persona::kMaidManager:
      return maid_manager_service_.HandleMessage(wrapper_tuple, routing_message.sender,
                                                 routing_message.receiver);
    case nfs::Persona::kVersionManager:
      return version_manager_service_.HandleMessage(wrapper_tuple, routing_message.sender,
                                                    routing_message.receiver);
    case nfs::Persona::kDataManager:
      return data_manager_service_.HandleMessage(wrapper_tuple, routing_message.sender,
                                                 routing_message.receiver);
    case nfs::Persona::kPmidManager:
      return pmid_manager_service_.HandleMessage(wrapper_tuple, routing_message.sender,
                                                 routing_message.receiver);
    case nfs::Persona::kPmidNode:
      return pmid_node_service_.HandleMessage(wrapper_tuple, routing_message.sender,
                                              routing_message.receiver);
    default:
      LOG(kError) << "Unhandled Persona";
  }
}

template<typename T>
bool Demultiplexer::GetFromCache(const T& serialised_message) {
  auto wrapper_tuple(nfs::ParseMessageWrapper(serialised_message.contents));
  return pmid_node_service_.GetFromCache(wrapper_tuple,
                                         serialised_message.sender,
                                         serialised_message.receiver);
}

template<typename T>
void Demultiplexer::StoreInCache(const T& serialised_message) {
  auto wrapper_tuple(nfs::ParseMessageWrapper(serialised_message.contents));
  pmid_node_service_.StoreInCache(wrapper_tuple,
                                  serialised_message.sender,
                                  serialised_message.receiver);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DEMULTIPLEXER_H_
