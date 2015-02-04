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

#ifndef MAIDSAFE_VAULT_DEMULTIPLEXER_H_
#define MAIDSAFE_VAULT_DEMULTIPLEXER_H_

#include <string>
#include <type_traits>

#include "maidsafe/common/log.h"
#include "maidsafe/common/types.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/message_wrapper.h"
#include "maidsafe/nfs/message_types.h"
#include "maidsafe/vault/message_types.h"
#include "maidsafe/nfs/service.h"

#include "maidsafe/vault/data_manager/service.h"
#include "maidsafe/vault/maid_manager/service.h"
#include "maidsafe/vault/pmid_manager/service.h"
#include "maidsafe/vault/mpid_manager/service.h"
#include "maidsafe/vault/pmid_node/service.h"
#include "maidsafe/vault/version_handler/service.h"
#include "maidsafe/vault/cache_handler/service.h"

namespace maidsafe {

namespace vault {

class Demultiplexer {
 public:
  Demultiplexer(nfs::Service<MaidManagerService>& maid_manager_service,
                nfs::Service<VersionHandlerService>& version_handler_service,
                nfs::Service<DataManagerService>& data_manager_service,
                nfs::Service<PmidManagerService>& pmid_manager_service,
                nfs::Service<PmidNodeService>& pmid_node_service,
                nfs::Service<MpidManagerService>& mpid_manager_service,
                nfs_client::DataGetter& data_getter);
  template <typename T>
  void HandleMessage(const T& routing_message);

 private:
  nfs::Service<MaidManagerService>& maid_manager_service_;
  nfs::Service<VersionHandlerService>& version_handler_service_;
  nfs::Service<DataManagerService>& data_manager_service_;
  nfs::Service<PmidManagerService>& pmid_manager_service_;
  nfs::Service<PmidNodeService>& pmid_node_service_;
  nfs::Service<MpidManagerService>& mpid_manager_service_;
  nfs_client::DataGetter& data_getter_;
};

template <typename T>
void Demultiplexer::HandleMessage(const T& routing_message) {
  nfs::TypeErasedMessageWrapper wrapper_tuple;
  try {
    wrapper_tuple = nfs::ParseMessageWrapper(routing_message.contents);
  }
  catch (const maidsafe_error& error) {
    if (error.code() == make_error_code(CommonErrors::parsing_error))
      return;
    throw;
  }
  const auto& destination_persona(std::get<2>(wrapper_tuple));
  LOG(kVerbose) << "Demultiplexer::HandleMessage Persona data : " << destination_persona.data;
  static_assert(std::is_same<decltype(destination_persona),
                             const nfs::detail::DestinationTaggedValue&>::value,
                "The value retrieved from the tuple isn't the destination type, but should be.");
  switch (destination_persona.data) {
    case nfs::Persona::kMaidManager:
      return maid_manager_service_.HandleMessage(wrapper_tuple, routing_message.sender,
                                                 routing_message.receiver);
    case nfs::Persona::kVersionHandler:
      return version_handler_service_.HandleMessage(wrapper_tuple, routing_message.sender,
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
    case nfs::Persona::kMpidManager:
      return mpid_manager_service_.HandleMessage(wrapper_tuple, routing_message.sender,
                                                 routing_message.receiver);
    case nfs::Persona::kDataGetter:
      return data_getter_.service().HandleMessage(wrapper_tuple, routing_message.sender,
                                                  routing_message.receiver);
    default:
      LOG(kError) << "Persona data : " << destination_persona.data << " is an Unhandled Persona ";
  }
}

template <>
void Demultiplexer::HandleMessage(const routing::SingleToGroupRelayMessage& routing_message);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DEMULTIPLEXER_H_
