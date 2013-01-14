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

#include "maidsafe/vault/maid_account_holder/maid_account_holder.h"

#include "maidsafe/vault/maid_account_holder/maid_account.h"


namespace maidsafe {

namespace vault {

namespace detail {

bool NodeRangeCheck(routing::Routing& routing, const NodeId& node_id) {
  return routing.IsNodeIdInGroupRange(node_id);  // provisional call to Is..
}

}  // namespace detail

MaidAccountHolder::MaidAccountHolder(const passport::Pmid& pmid,
                                     routing::Routing& routing,
                                     nfs::PublicKeyGetter& public_key_getter,
                                     const boost::filesystem::path& vault_root_dir)
    : routing_(routing),
      kRootDir_(vault_root_dir / "maids"),
      nfs_(routing, pmid),
      public_key_getter_(public_key_getter),
      maid_account_handler_(vault_root_dir)/*,
      maid_accounts_()*/ {
  boost::filesystem::exists(kRootDir_) || boost::filesystem::create_directory(kRootDir_);

  boost::filesystem::directory_iterator end_iter;
  for (boost::filesystem::directory_iterator dir_iter(kRootDir_);
       dir_iter != end_iter;
       ++dir_iter) {
    if (!boost::filesystem::is_regular_file(dir_iter->status()))
      continue;
    std::string account_content;
    if (!ReadFile(*dir_iter, &account_content))
      continue;
//    maidsafe::vault::MaidAccount maid_account((NonEmptyString(account_content)));
//    if (dir_iter->path().filename().string() == maid_account.maid_name()->string()) {
//      maid_accounts_.push_back(maid_account);
//    } else {
//      if (!boost::filesystem::remove(*dir_iter))  // can throw!
//        LOG(kError) << "Failed to remove corrupt file " << *dir_iter;
//    }
  }
}

//void MaidAccountHolder::Serialise() {
//  for (auto& account : maid_accounts_)
//    WriteFile(kRootDir_ / account.maid_name().data.string(), account.Serialise().string());
//}

//void MaidAccountHolder::Serialise(const passport::Maid& maid) {
//  auto itr = maid_accounts_.begin();
//  while (itr != maid_accounts_.end()) {
//    if ((*itr).maid_name().data.string() == maid.name().data.string()) {
//      WriteFile(kRootDir_ / (*itr).maid_name().data.string(), (*itr).Serialise().string());
//      break;
//    }
//    ++itr;
//  }
//}

//void MaidAccountHolder::RemoveAccount(const passport::Maid& maid) {
//  auto itr = maid_accounts_.begin();
//  while (itr != maid_accounts_.end()) {
//    if ((*itr).maid_name().data.string() == maid.name().data.string()) {
//      boost::filesystem::remove(kRootDir_ / (*itr).maid_name().data.string());
//      break;
//    }
//    ++itr;
//  }
//}

// bool MaidAccountHolder::HandleNewComer(const passport::/*PublicMaid*/PublicPmid& p_maid) {
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
// bool MaidAccountHolder::OnKeyFetched(const passport::/*PublicMaid*/PublicPmid& p_maid,
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

// bool MaidAccountHolder::HandleNewComer(const nfs::PmidRegistration& pmid_registration) {
//   Identity maid_id(pmid_registration.maid_id());
//   MaidAccount maid_account(maid_id);
//   maid_account.PushPmidTotal(PmidTotals(pmid_registration, PmidRecord(maid_id)));
//   return WriteFile(kRootDir_ / maid_id.string(), maid_account.Serialise().string());
// }


MaidAccountHolder::~MaidAccountHolder() {}

// void MaidAccountHolder::CloseNodeReplaced(
//     const std::vector<routing::NodeInfo>& /*new_close_nodes*/) {
//   SendSyncData();
// }

void MaidAccountHolder::SendSyncData() {
//  for (auto& account : maid_accounts_) {
//    nfs::PostMessage message(nfs::PostActionType::kSynchronise,
//                             nfs::PersonaType::kMaidAccountHolder,
//                             nfs::Message::Source(nfs::PersonaType::kMaidAccountHolder,
//                                                  routing_.kNodeId()),
//                             account.maid_name(),
//                             account.Serialise(),
//                             maidsafe::rsa::Signature());
//    nfs_.PostSyncData(message, [this](nfs::PostMessage message) {
//                                  this->OnPostErrorHandler(message);
//                               });
//  }
}

bool MaidAccountHolder::HandleReceivedSyncData(const NonEmptyString &/*serialised_account*/) {
//  MaidAccount maid_account(serialised_account);
//  return WriteFile(kRootDir_ / maid_account.maid_name().data.string(), serialised_account.string());
  return false;
}

void MaidAccountHolder::HandlePostMessage(const nfs::PostMessage& message,
                                          const routing::ReplyFunctor& reply_functor) {
// HandleNewComer(p_maid);
  nfs::PostActionType action_type(message.post_action_type());
  NodeId source_id(message.source().node_id);
  switch (action_type) {
    case nfs::PostActionType::kRegisterPmid:
      break;
    case nfs::PostActionType::kConnect:
      break;
    case nfs::PostActionType::kGetPmidSize:
      break;
    case nfs::PostActionType::kNodeDown:
      break;
    case nfs::PostActionType::kSynchronise:
      if (detail::NodeRangeCheck(routing_, source_id)) {
        HandleReceivedSyncData(message.content());
      } else {
        reply_functor(nfs::ReturnCode(-1).Serialise()->string());
      }
      break;
    default:
      LOG(kError) << "Unhandled Post action type";
  }
}

void MaidAccountHolder::OnPostErrorHandler(nfs::PostMessage /*message*/) {
  // TODO(Team): BEFORE_RELEASE implement
}

}  // namespace vault

}  // namespace maidsafe
