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

#include "maidsafe/vault/pmid_manager/service.h"

#include "maidsafe/common/error.h"

#include "maidsafe/vault/pmid_manager/pmid_manager.pb.h"
#include "maidsafe/vault/pmid_manager/metadata.h"
#include "maidsafe/vault/sync.pb.h"

namespace fs = boost::filesystem;

namespace maidsafe {
namespace vault {

const int PmidManagerService::kDeleteRequestsRequired_(3);
const int PmidManagerService::kPutRepliesSuccessesRequired_(1);

namespace detail {

PmidName GetPmidAccountName(const nfs::Message& message) {
  return PmidName(Identity(message.data().name));
}

template<typename Message>
inline bool ForThisPersona(const Message& message) {
  return message.destination_persona() != nfs::Persona::kPmidManager;
}

}  // namespace detail

PmidManagerService::PmidManagerService(const passport::Pmid& pmid,
                                                   routing::Routing& routing,
                                                   Db& db)
    : routing_(routing),
      accumulator_mutex_(),
      accumulator_(),
      pmid_account_handler_(db, routing.kNodeId()),
      nfs_(routing, pmid) {}

void PmidManagerService::HandleMessage(const nfs::Message& message,
                                       const routing::ReplyFunctor& /*reply_functor*/) {
  ValidateGenericSender(message);
  nfs::Reply reply(CommonErrors::success);
  nfs::MessageAction action(message.data().action);
  switch (action) {
    case nfs::MessageAction::kSynchronise:
      return HandleSync(message);
    case nfs::MessageAction::kAccountTransfer:
      return HandleAccountTransfer(message);
    case nfs::MessageAction::kGetPmidTotals:
      return GetPmidTotals(message);
    case nfs::MessageAction::kGetPmidAccount:
      return GetPmidAccount(message);
    default:
      LOG(kError) << "Unhandled Post action type";
  }
}

void PmidManagerService::CreatePmidAccount(const nfs::Message& message) {
  try {
    pmid_account_handler_.CreateAccount(message.pmid_node());
  }
  catch(const maidsafe_error& error) {
    LOG(kWarning) << error.what();
  }
  catch(...) {
    LOG(kError) << "Unknown error.";
  }
}

void PmidManagerService::GetPmidTotals(const nfs::Message& message) {
  try {
    PmidManagerMetadata metadata(pmid_account_handler_.GetMetadata(PmidName(message.data().name)));
    if (!metadata.pmid_name.data.string().empty()) {
      nfs::Reply reply(CommonErrors::success, metadata.Serialise());
      nfs_.ReturnPmidTotals(message.source().node_id, reply.Serialise());
    } else {
      nfs_.ReturnFailure(message);
    }
  }
  catch(const maidsafe_error& error) {
    LOG(kWarning) << error.what();
  }
  catch(...) {
    LOG(kWarning) << "Unknown error.";
  }
}

void PmidManagerService::GetPmidAccount(const nfs::Message& message) {
  try {
    PmidName pmid_name(detail::GetPmidAccountName(message));
    protobuf::PmidAccountResponse pmid_account_response;
    protobuf::PmidAccount pmid_account;
    PmidAccount::serialised_type serialised_account_details;
    pmid_account.set_pmid_name(pmid_name.data.string());
    try {
      serialised_account_details = pmid_account_handler_.GetSerialisedAccount(pmid_name, false);
      pmid_account.set_serialised_account_details(serialised_account_details.data.string());
      pmid_account_response.set_status(true);
    }
    catch(const maidsafe_error&) {
      pmid_account_response.set_status(false);
      pmid_account_handler_.CreateAccount(PmidName(detail::GetPmidAccountName(message)));
    }
    pmid_account_response.mutable_pmid_account()->CopyFrom(pmid_account);
    nfs_.AccountTransfer<passport::Pmid>(
          pmid_name, NonEmptyString(pmid_account_response.SerializeAsString()));
  }
  catch(const maidsafe_error& error) {
    LOG(kWarning) << error.what();
  }
  catch(...) {
    LOG(kWarning) << "Unknown error.";
  }
}

void PmidManagerService::HandleChurnEvent(std::shared_ptr<routing::MatrixChange> matrix_change) {
  auto account_names(pmid_account_handler_.GetAccountNames());
  auto itr(std::begin(account_names));
  while (itr != std::end(account_names)) {
    auto check_holders_result(matrix_change->CheckHolders(NodeId((*itr)->string())));
    // Delete accounts for which this node is no longer responsible.
    if (check_holders_result.proximity_status != routing::GroupRangeStatus::kInRange) {
      pmid_account_handler_.DeleteAccount(*itr);
      itr = account_names.erase(itr);
      continue;
    }
    // Replace old_node(s) in sync object and send AccountTransfer to new node(s).
    assert(check_holders_result.old_holders.size() == check_holders_result.new_holders.size());
    for (auto i(0U); i != check_holders_result.old_holders.size(); ++i) {
      pmid_account_handler_.ReplaceNodeInSyncList(*itr, check_holders_result.old_holders[i],
                                                  check_holders_result.new_holders[i]);
      TransferAccount(*itr, check_holders_result.new_holders[i]);
    }

    ++itr;
  }
}

void PmidManagerService::ValidateDataSender(const nfs::Message& message) const {
  if (!message.HasDataHolder()
      || !routing_.IsConnectedVault(NodeId(message.pmid_node()->string()))
      || routing_.EstimateInGroup(message.source().node_id, NodeId(message.data().name)))
    ThrowError(VaultErrors::permission_denied);

  if (!FromDataManager(message) || !detail::ForThisPersona(message))
    ThrowError(CommonErrors::invalid_parameter);
}

void PmidManagerService::ValidateGenericSender(const nfs::Message& message) const {
  if (!routing_.IsConnectedVault(message.source().node_id)
      || routing_.EstimateInGroup(message.source().node_id, NodeId(message.data().name)))
    ThrowError(VaultErrors::permission_denied);

  if (!FromDataManager(message) || !FromPmidNode(message) || !detail::ForThisPersona(message))
    ThrowError(CommonErrors::invalid_parameter);
}

// =============== Sync ===========================================================================

void PmidManagerService::Sync(const PmidName& account_name) {
  auto serialised_sync_data(pmid_account_handler_.GetSyncData(account_name));
  if (!serialised_sync_data.IsInitialised())  // Nothing to sync
    return;

  protobuf::Sync proto_sync;
  proto_sync.set_account_name(account_name->string());
  proto_sync.set_serialised_unresolved_entries(serialised_sync_data.string());

  nfs_.Sync(account_name, NonEmptyString(proto_sync.SerializeAsString()));
}

void PmidManagerService::HandleSync(const nfs::Message& message) {
  std::vector<PmidManagerUnresolvedEntry> resolved_entries;
  protobuf::Sync proto_sync;
  if (!proto_sync.ParseFromString(message.data().content.string())) {
    LOG(kError) << "Error parsing Synchronise message.";
    return;
  }

  pmid_account_handler_.ApplySyncData(PmidName(Identity(proto_sync.account_name())),
                                      NonEmptyString(proto_sync.serialised_unresolved_entries()));
}

// =============== Account transfer ===============================================================

void PmidManagerService::TransferAccount(const PmidName& account_name, const NodeId& new_node) {
  protobuf::PmidAccount pmid_account;
  pmid_account.set_pmid_name(account_name.data.string());
  PmidAccount::serialised_type
    serialised_account_details(pmid_account_handler_.GetSerialisedAccount(account_name, true));
  pmid_account.set_serialised_account_details(serialised_account_details.data.string());
  nfs_.TransferAccount(new_node, NonEmptyString(pmid_account.SerializeAsString()));
}

void PmidManagerService::HandleAccountTransfer(const nfs::Message& message) {
  protobuf::PmidAccount pmid_account;
  NodeId source_id(message.source().node_id);
  if (!pmid_account.ParseFromString(message.data().content.string()))
    return;

  PmidName account_name(Identity(pmid_account.pmid_name()));
  bool finished_all_transfers(
      pmid_account_handler_.ApplyAccountTransfer(account_name, source_id,
         PmidAccount::serialised_type(NonEmptyString(pmid_account.serialised_account_details()))));
  if (finished_all_transfers)
    return;    // TODO(Team) Implement whatever else is required here?
}

}  // namespace vault
}  // namespace maidsafe
