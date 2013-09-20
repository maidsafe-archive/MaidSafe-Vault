/*  Copyright 2013 MaidSafe.net limited

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

#ifndef MAIDSAFE_VAULT_PMID_NODE_DISPATCHER_H_
#define MAIDSAFE_VAULT_PMID_NODE_DISPATCHER_H_

#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/message_types.h"
#include "maidsafe/vault/messages.h"
#include "maidsafe/vault/types.h"

namespace maidsafe {

namespace vault {

class PmidNodeDispatcher {
 public:
  PmidNodeDispatcher(routing::Routing& routing);

  void SendGetRequest(const nfs_vault::DataName& data_name);
  void SendPmidAccountRequest();

  template<typename Data>
  void SendPutRespnse(const Data& data,
                      const nfs::MessageId& message_id,
                      const maidsafe_error& error);
 private:
  PmidNodeDispatcher();
  PmidNodeDispatcher(const PmidNodeDispatcher&);
  PmidNodeDispatcher(PmidNodeDispatcher&&);
  PmidNodeDispatcher& operator=(PmidNodeDispatcher);

  routing::GroupSource Sender(const MaidName& account_name) const;

  routing::Routing& routing_;
};

template<typename Data>
void PmidNodeDispatcher::SendPutRespnse(const Data& data,
                                        const nfs::MessageId& message_id,
                                        const maidsafe_error& error) {
  typedef nfs::PutResponseFromPmidNodeToPmidManager NfsMessage;
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
  NfsMessage nfs_message;
  if (error.code() != CommonErrors::success) {
    nfs_message = NfsMessage(message_id,
                             nfs_client::DataNameAndContentAndReturnCode(
                                 data.name().type,
                                 data.name()->name.raw_name,
                                 nfs_client::ReturnCode(error),
                                 data.data()));
  } else {
    nfs_message = NfsMessage(message_id,
                             nfs_client::DataNameAndContentAndReturnCode(
                                 data.name().type,
                                 data.name()->name.raw_name,
                                 nfs_client::ReturnCode(error)));
  }
  RoutingMessage routing_message(nfs_message.Serialise(),
                                 NfsMessage::Sender(routing::SingleId(routing_.kNodeId())),
                                 NfsMessage::Receiver(routing::GroupId(routing_.kNodeId())));
  routing_.Send(routing_message);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_NODE_DISPATCHER_H_
