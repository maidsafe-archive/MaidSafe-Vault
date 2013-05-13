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

#include "maidsafe/vault/structured_data_manager/structured_data_manager_service.h"

#include <string>

#include "maidsafe/nfs/pmid_registration.h"

#include "maidsafe/vault/sync.h"


namespace maidsafe {

namespace vault {


namespace {

template<typename Message>
inline bool ForThisPersona(const Message& message) {
  return message.destination_persona() != nfs::Persona::kStructuredDataManager;
}

}  // unnamed namespace

namespace detail {

vault::StructuredDataManagerService::AccountName GetStructuredDataAccountName(
    const nfs::Message& message) {
  return vault::StructuredDataManagerService::AccountName(
      std::make_pair(Identity(message.source().node_id.string()),
                     Identity(message.data_holder().data.string())));
}

}  // namespace detail



// const int StructuredDataManagerService::kPutRepliesSuccessesRequired_(3);

StructuredDataManagerService::StructuredDataManagerService(const passport::Pmid& pmid,
                                                   routing::Routing& routing,
                                                   nfs::PublicKeyGetter& public_key_getter,
                                                   const boost::filesystem::path& path)
    : routing_(routing),
      public_key_getter_(public_key_getter),
      accumulator_mutex_(),
      account_name_(std::make_pair(Identity(routing_.kNodeId().string()),
                                   Identity(pmid.name().data.string()))),
      accumulator_(),
      structured_data_db_(path),
      nfs_(routing_, pmid) {}


// void StructuredDataManagerService::ValidateSender(const nfs::Message& message) const {
//   if (!routing_.IsConnectedClient(message.source().node_id))
//     ThrowError(VaultErrors::permission_denied);
//
//   if (!FromClientMaid(message) || !ForThisPersona(message))
//     ThrowError(CommonErrors::invalid_parameter);
// }
//
// void StructuredDataManagerService::ValidateSender(const nfs::Message& message) const {
//   if (!routing_.IsConnectedVault(message.source().node_id))
//     ThrowError(VaultErrors::permission_denied);
//   if (!FromStructuredDataManager(message) || !ForThisPersona(message))
//     ThrowError(CommonErrors::invalid_parameter);
// }


// =============== Put/Delete data =================================================================

void StructuredDataManagerService::AddToAccumulator(const nfs::Message& message) {
  std::lock_guard<std::mutex> lock(accumulator_mutex_);
  accumulator_.SetHandled(message, maidsafe_error(CommonErrors::success));
}


// // =============== Sync ============================================================================
//
// void StructuredDataManagerService::Sync(const MaidName& account_name) {
//   auto serialised_sync_data(maid_account_handler_.GetSyncData(account_name));
//   if (!serialised_sync_data.IsInitialised())  // Nothing to sync
//     return;
//
//   protobuf::Sync proto_sync;
//   proto_sync.set_account_name(account_name->string());
//   proto_sync.set_serialised_unresolved_entries(serialised_sync_data.string());
//
//   nfs_.Sync(account_name, NonEmptyString(proto_sync.SerializeAsString()));
//   // TODO(Fraser#5#): 2013-05-03 - Check this is correct place to increment sync attempt counter.
//   maid_account_handler_.IncrementSyncAttempts(account_name);
// }
//
// void StructuredDataManagerService::HandleSync(const nfs::Message& message) {
//   protobuf::Sync proto_sync;
//   if (!proto_sync.ParseFromString(message.content().string())) {
//     LOG(kError) << "Error parsing kSynchronise message.";
//     return;
//   }
//   maid_account_handler_.ApplySyncData(MaidName(Identity(proto_sync.account_name())),
//                                       NonEmptyString(proto_sync.serialised_unresolved_entries()));
// }
//
//
// // =============== Account transfer ================================================================
//
// void StructuredDataManagerService::TransferAccount(const MaidName& account_name,
//                                                const NodeId& new_node) {
//   protobuf::MaidAccount maid_account;
//   maid_account.set_maid_name(account_name->string());
//   maid_account.set_serialised_account_details(
//       maid_account_handler_.GetSerialisedAccount(account_name)->string());
//   nfs_.TransferAccount(new_node, NonEmptyString(maid_account.SerializeAsString()));
// }
//
// void StructuredDataManagerService::HandleAccountTransfer(const nfs::Message& message) {
//   protobuf::MaidAccount maid_account;
//   NodeId source_id(message.source().node_id);
//   if (!maid_account.ParseFromString(message.content().string()))
//     return;
//
//   MaidName account_name(Identity(maid_account.maid_name()));
//   bool finished_all_transfers(
//       maid_account_handler_.ApplyAccountTransfer(account_name, source_id,
//           MaidAccount::serialised_type(NonEmptyString(maid_account.serialised_account_details()))));
//   if (finished_all_transfers)
//     UpdatePmidTotals(account_name);
// }


}  // namespace vault

}  // namespace maidsafe
