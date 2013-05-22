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

#ifndef MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_MANAGER_SERVICE_INL_H_
#define MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_MANAGER_SERVICE_INL_H_

#include <exception>
#include <string>

#include "maidsafe/common/error.h"
#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/common/on_scope_exit.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/nfs/utils.h"
#include "maidsafe/nfs/reply.h"

#include "maidsafe/vault/utils.h"


namespace maidsafe {

namespace vault {

namespace detail {

vault::StructuredDataManagerService::SDMKey GetStructuredDataKey(const nfs::Message& message);

template<typename Data>
typename Data::name_type GetStructuredDataName(const nfs::Message& message) {
  return typename Data::name_type(message.data().name);
}

// template<typename Data, nfs::MessageAction action>
// StructuredDataManagerUnresolvedEntry CreateUnresolvedEntry(const nfs::Message& message,
//                                                            Identity value,
//                                                            const NodeId& this_id) {
//   static_assert(action == nfs::MessageAction::kPut || action == nfs::MessageAction::kDelete,
//                 "Action must be either kPut of kDelete.");
//   return StructuredDataManagerUnresolvedEntry(
//       std::make_pair(DataNameVariant(GetDataName<Data>(message)), action), value, this_id);
// }

}  // namespace detail


template<typename Data>
void StructuredDataManagerService::HandleMessage(const nfs::Message& message,
                                                 const routing::ReplyFunctor& reply_functor) {
//   ValidateSender(message);
  nfs::Reply reply(CommonErrors::success);
  {
    std::lock_guard<std::mutex> lock(accumulator_mutex_);
    if (accumulator_.CheckHandled(message, reply))
      return reply_functor(reply.Serialise()->string());
  }

  if (message.data().action == nfs::MessageAction::kPut) {
    HandlePut<Data>(message);
  } else if (message.data().action == nfs::MessageAction::kDeleteBranchUntilFork) {
    HandleDeleteBranchUntilFork<Data>(message);
  } else if (message.data().action == nfs::MessageAction::kGet) {
    HandleGet<Data>(message, reply_functor);
  }else if (message.data().action == nfs::MessageAction::kGetBranch) {
    HandleGetBranch<Data>(message, reply_functor);
  }
;
}
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
template<typename Data>
void StructuredDataManagerService::HandlePut(const nfs::Message& message) {
  maidsafe_error return_code(CommonErrors::success);
  try {
    Data data(typename Data::name_type(message.data().name),
              typename Data::serialised_type(message.data().content));
    auto account_name(detail::GetStructuredDataKey(message));

  }
  catch(const maidsafe_error& error) {
    LOG(kWarning) << error.what();
    return_code = error;
  }
  catch(...) {
    LOG(kWarning) << "Unknown error.";
    return_code = MakeError(CommonErrors::unknown);
  }
}

//template<typename Data>
//void StructuredDataManagerService::HandleDelete(const nfs::Message& message) {
//  try {
//    auto data_name(detail::GetStructuredDataName<Data>(message));
//    //DeleteFromAccount<Data>(detail::GetSourceMaidName(message), data_name, version);
//  }
//  catch(const maidsafe_error& error) {
//    LOG(kWarning) << error.what();
//  }
//  catch(...) {
//    LOG(kWarning) << "Unknown error.";
//   }
//}

// template<typename Data, nfs::MessageAction action>
// void StructuredDataManagerService::AddLocalUnresolvedEntryThenSync(const nfs::Message& message,
//                                                                    int32_t cost) {
//   auto account_name(detail::GetMaidAccountName(message));
//   auto unresolved_entry(detail::CreateUnresolvedEntry<Data, action>(message, cost,
//                                                                     routing_.kNodeId()));
//   maid_account_handler_.AddLocalUnresolvedEntry(account_name, unresolved_entry);
//   Sync(account_name);
// }

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_MANAGER_SERVICE_INL_H_
