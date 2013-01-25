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

#include "maidsafe/vault/maid_account_holder_service.h"

#include "boost/filesystem/operations.hpp"


namespace fs = boost::filesystem;

namespace maidsafe {

namespace vault {

MaidAccountHolderService::MaidAccountHolderService(const passport::Pmid& pmid,
                                                   routing::Routing& routing,
                                                   nfs::PublicKeyGetter& public_key_getter,
                                                   const fs::path& vault_root_dir)
    : routing_(routing),
      public_key_getter_(public_key_getter),
      accumulator_(),
      maid_account_handler_(vault_root_dir),
      nfs_(routing, pmid) {}

void MaidAccountHolderService::ValidateDataMessage(const nfs::DataMessage& data_message) {
  if (!routing_.IsConnectedToClient(data_message.source().node_id))
    ThrowError(VaultErrors::permission_denied);
}

void MaidAccountHolderService::HandleGenericMessage(const nfs::GenericMessage& generic_message,
                                                    const routing::ReplyFunctor& reply_functor) {
// HandleNewComer(p_maid);
  nfs::GenericMessage::Action action(generic_message.action());
  NodeId source_id(generic_message.source().node_id);
  switch (action) {
    case nfs::GenericMessage::Action::kRegisterPmid:
      break;
    case nfs::GenericMessage::Action::kConnect:
      break;
    case nfs::GenericMessage::Action::kGetPmidSize:
      break;
    case nfs::GenericMessage::Action::kNodeDown:
      break;
    case nfs::GenericMessage::Action::kSynchronise:
      if (routing_.IsConnectedToClient(source_id)) {
        HandleReceivedSyncData(generic_message.content());
      } else {
        reply_functor(nfs::ReturnCode(-1).Serialise()->string());
      }
      break;
    default:
      LOG(kError) << "Unhandled Post action type";
  }
}

// bool MaidAccountHolderService::HandleNewComer(const passport::/*PublicMaid*/PublicPmid& p_maid) {
//   std::promise<bool> result_promise;
//   std::future<bool> result_future = result_promise.get_future();
//   auto get_key_future([this, p_maid, &result_promise] (
//       std::future<maidsafe::passport::PublicPmid> key_future) {
//     try {
//       maidsafe::passport::PublicPmid p_pmid = key_future.get();
//       result_promise.set_value(OnKeyFetched(p_maid, p_pmid));
//     }
//     catch(const std::exception& ex) {
//       LOG(kError) << "Failed to get key for " << HexSubstr(p_maid.name().data.string())
//                   << " : " << ex.what();
//       result_promise.set_value(false);
//     }
//   });
//   public_key_getter_.HandleGetKey<maidsafe::passport::PublicPmid>(p_maid.name(), get_key_future);
//   return result_future.get();
// }
//
// bool MaidAccountHolderService::OnKeyFetched(const passport::/*PublicMaid*/PublicPmid& p_maid,
//                                      const passport::PublicPmid& p_pmid) {
//   if (!asymm::CheckSignature(asymm::PlainText(asymm::EncodeKey(p_pmid.public_key())),
//                              p_pmid.validation_token(), p_maid.public_key())) {
//     LOG(kError) << "Fetched pmid for " << HexSubstr(p_maid.name().data.string())
//                 << " contains invalid token";
//     return false;
//   }
//
//   maidsafe::nfs::MaidAccount maid_account(p_maid.name().data);
//   maid_account.PushPmidTotal(nfs::PmidTotals(nfs::PmidRegistration(p_maid.name().data,
//                                                                   p_pmid.name().data,
//                                                                   false,
//                                                                   p_maid.validation_token(),
//                                                                   p_pmid.validation_token()),
//                                             nfs::PmidSize(p_pmid.name().data)));
//   return WriteFile(kRootDir_ / maid_account.maid_id().string(),
//                    maid_account.Serialise().string());
// }

// bool MaidAccountHolderService::HandleNewComer(const nfs::PmidRegistration& pmid_registration) {
//   Identity maid_id(pmid_registration.maid_id());
//   MaidAccount maid_account(maid_id);
//   maid_account.PushPmidTotal(PmidTotals(pmid_registration, PmidRecord(maid_id)));
//   return WriteFile(kRootDir_ / maid_id.string(), maid_account.Serialise().string());
// }


// void MaidAccountHolderService::HandleSynchronise(
//     const std::vector<routing::NodeInfo>& new_close_nodes) {
//   SendSyncData();
// }

void MaidAccountHolderService::SendSyncData() {
//  for (auto& account : maid_accounts_) {
//    nfs::GenericMessage generic_message(
//        nfs::GenericMessage::Action::kSynchronise,
//        nfs::Persona::kMaidAccountHolder,
//        nfs::MessageSource(nfs::Persona::kMaidAccountHolder, routing_.kNodeId()),
//        account.maid_name().data,
//        account.Serialise());
//    nfs_.PostSyncData(generic_message,
//        [this](nfs::GenericMessage message) { this->OnPostErrorHandler(message); });
//  }
}

bool MaidAccountHolderService::HandleReceivedSyncData(
    const NonEmptyString &/*serialised_account*/) {
//  MaidAccount maid_account(serialised_account);
//  return WriteFile(kRootDir_ / maid_account.maid_name().data.string(),
//                   serialised_account.string());
  return false;
}

}  // namespace vault

}  // namespace maidsafe
