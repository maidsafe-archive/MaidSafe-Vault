/*  Copyright 2012 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.novinet.com/license

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#include "maidsafe/vault/data_manager/service.h"

#include <string>
#include <vector>

#include "maidsafe/routing/parameters.h"
#include "maidsafe/nfs/utils.h"


namespace maidsafe {

namespace vault {

namespace {

inline bool SenderInGroupForClientMaid(const nfs::Message& message, routing::Routing& routing) {
  return routing.EstimateInGroup(message.source().node_id,
                                 NodeId(message.client_validation().name.string()));
}

template<typename Message>
inline bool ForThisPersona(const Message& message) {
  return message.destination_persona() != nfs::Persona::kDataManager;
}

}  // unnamed namespace

namespace detail {
void SendMetadataCost(const nfs::Message& original_message,
                      const routing::ReplyFunctor& reply_functor,
                      nfs::Reply& reply) {
  if (!reply_functor)
    return;
  if (!reply.IsSuccess())
    reply = nfs::Reply(reply.error(), original_message.Serialise().data);

  reply_functor(reply.Serialise()->string());
}

}  // namspace detail

const int DataManagerService::kPutRequestsRequired_(3);
const int DataManagerService::kStateChangesRequired_(3);
const int DataManagerService::kDeleteRequestsRequired_(3);


DataManagerService::DataManagerService(const passport::Pmid& pmid,
                                               routing::Routing& routing,
                                               nfs::PublicKeyGetter& public_key_getter)
    : routing_(routing),
      public_key_getter_(public_key_getter),
      accumulator_mutex_(),
      accumulator_(),
      metadata_handler_(routing.kNodeId()),
      nfs_(routing, pmid) {}

void DataManagerService::ValidatePutSender(const nfs::Message& message) const {
  if (!SenderInGroupForClientMaid(message, routing_) || !ThisVaultInGroupForData(message)) {
    ThrowError(VaultErrors::permission_denied);
  }

  if (!FromMaidManager(message) || !ForThisPersona(message))
    ThrowError(CommonErrors::invalid_parameter);
}

void DataManagerService::ValidatePutResultSender(const nfs::Message& message) const {
  // FIXME(Prakash) Need to pass PmidName in message to validate
  if (!FromPmidManager(message) || !ForThisPersona(message))
    ThrowError(CommonErrors::invalid_parameter);
}

void DataManagerService::ValidateGetSender(const nfs::Message& message) const {
  if (!(FromClientMaid(message) ||
          FromDataHolder(message) ||
          FromDataGetter(message) ||
          FromOwnerDirectoryManager(message) ||
          FromGroupDirectoryManager(message) ||
          FromWorldDirectoryManager(message)) ||
      !ForThisPersona(message)) {
    ThrowError(CommonErrors::invalid_parameter);
  }
}

void DataManagerService::ValidateDeleteSender(const nfs::Message& message) const {
  if (!SenderInGroupForClientMaid(message, routing_))
    ThrowError(VaultErrors::permission_denied);

  if (!FromMaidManager(message) || !ForThisPersona(message))
    ThrowError(CommonErrors::invalid_parameter);
}

void DataManagerService::ValidatePostSender(const nfs::Message& message) const {
  if (!(FromDataManager(message) || FromPmidManager(message)) ||
      !ForThisPersona(message)) {
    ThrowError(CommonErrors::invalid_parameter);
  }
}

//void DataManagerService::SendSyncData() {}

void DataManagerService::HandleNodeDown(const nfs::Message& /*message*/) {
  try {
    int online_holders(-1);
//    metadata_handler_.MarkNodeDown(message.name(), PmidName(), online_holders);
    if (online_holders < 3) {
      // TODO(Team): Get content. There is no manager available yet.

      // Select new holder
      NodeId new_holder(routing_.RandomConnectedNode());

      // TODO(Team): Put content. There is no manager available yet.
    }
  }
  catch(const std::exception &e) {
    LOG(kError) << "HandleNodeDown - Dropping process after exception: " << e.what();
    return;
  }
}

void DataManagerService::HandleNodeUp(const nfs::Message& /*message*/) {
  //try {
  //  metadata_handler_.MarkNodeUp(message.name(),
  //                               PmidName(Identity(message.name().string())));
  //}
  //catch(const std::exception &e) {
  //  LOG(kError) << "HandleNodeUp - Dropping process after exception: " << e.what();
  //  return;
  //}
}

bool DataManagerService::ThisVaultInGroupForData(const nfs::Message& message) const {
  return routing::GroupRangeStatus::kInRange ==
         routing_.IsNodeIdInGroupRange(NodeId(message.data().name.string()));
}

// =============== Sync and Record transfer =====================================================

//void DataManagerService::Sync() {
//  auto unresolved_entries(metadata_handler_.GetSyncData());
//  for (const auto& unresolved_entry : unresolved_entries) {
//    nfs_.Sync(unresolved_entry.key.first.name(), unresolved_entry.Serialise());
//  }
//}

void DataManagerService::HandleSync(const nfs::Message& message) {
  metadata_handler_.ApplySyncData(NonEmptyString(message.data().content.string()));
}

void DataManagerService::TransferRecord(const DataNameVariant& record_name,
                                            const NodeId& new_node) {
  nfs_.TransferRecord(record_name, new_node, metadata_handler_.GetSerialisedRecord(record_name));
}

void DataManagerService::HandleRecordTransfer(const nfs::Message& message) {
  metadata_handler_.ApplyRecordTransfer(NonEmptyString(message.data().content.string()));
}

// =============== Churn ===========================================================================
void DataManagerService::HandleChurnEvent(std::shared_ptr<routing::MatrixChange> matrix_change) {
  auto record_names(metadata_handler_.GetRecordNames());
  auto itr(std::begin(record_names));
  auto name(itr->name());
  while (itr != std::end(record_names)) {
    auto result(boost::apply_visitor(GetTagValueAndIdentityVisitor(), name));
    auto check_holders_result(matrix_change->(NodeId(result.second)));
    // Delete records for which this node is no longer responsible.
    if (check_holders_result.proximity_status != routing::GroupRangeStatus::kInRange) {
      metadata_handler_.DeleteRecord(itr->name());
      itr = record_names.erase(itr);
      continue;
    }

    // Replace old_node(s) in sync object and send TransferRecord to new node(s).
    assert(check_holders_result.old_holders.size() == check_holders_result.new_holders.size());
    for (auto i(0U); i != check_holders_result.old_holders.size(); ++i) {
      metadata_handler_.ReplaceNodeInSyncList(itr->name(), check_holders_result.old_holders[i],
                                              check_holders_result.new_holders[i]);
      TransferRecord(itr->name(), check_holders_result.new_holders[i]);
    }
    ++itr;
  }
  // TODO(Prakash):  modify ReplaceNodeInSyncList to be called once with vector of tuple/struct
  // containing record name, old_holders, new_holders.
}

template<>
void DataManagerService::HandleMessage(
   const nfs::PutRequestFromMaidManagerToDataManager& message,
   const typename nfs::PutRequestFromMaidManagerToDataManager::Sender& sender,
   const typename nfs::PutRequestFromMaidManagerToDataManager::Receiver& receiver) {
  typedef nfs::PutRequestFromMaidManagerToDataManager MessageType;
  OperationHandlerWrapper<DataManagerService, MessageType, nfs::DataManagerServiceMessages>(
      accumulator_,
      [this](const MessageType& message, const typename MessageType::Sender& sender) {
        return this->ValidateSender(message, sender);
      },
      Accumulator<nfs::DataManagerServiceMessages>::AddRequestChecker(RequiredRequests(sender)),
      this,
      accumulator_mutex_)(message, sender, receiver);
}

}  // namespace vault

}  // namespace maidsafe
