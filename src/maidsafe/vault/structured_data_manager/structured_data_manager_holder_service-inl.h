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

#ifndef MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_MANAGER_SERVICE_H_
#define MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_MANAGER_SERVICE_H_

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

AccountName GetAccountName(const nfs::DataMessage& data_message);

template<typename Data>
typename Data::name_type GetDataName(const nfs::DataMessage& data_message) {
  // Hash the data name to obfuscate the list of chunks associated with the client.
  return typename Data::name_type(data_message.data().name);
}

template<typename Data, nfs::MessageAction action>
StructuredDataManagerUnresolvedEntry CreateUnresolvedEntry(const nfs::DataMessage& data_message,
                                                           Identity value,
                                                           const NodeId& this_id) {
  static_assert(action == nfs::MessageAction::kPut || action == nfs::MessageAction::kDelete,
                "Action must be either kPut of kDelete.");
  return StructuredDataManagerUnresolvedEntry(
      std::make_pair(DataNameVariant(GetDataName<Data>(data_message)), action), value, this_id);
}

}  // namespace detail


template<typename Data>
void StructuredDataManagerService::HandleDataMessage(const nfs::DataMessage& data_message,
                                                 const routing::ReplyFunctor& reply_functor) {
  ValidateSender(data_message);
  nfs::Reply reply(CommonErrors::success);
  {
    std::lock_guard<std::mutex> lock(accumulator_mutex_);
    if (accumulator_.CheckHandled(data_message, reply))
      return;
  }

  if (data_message.data().action == nfs::DataMessage::Action::kPut) {
    HandlePut<Data>(data_message);
  } else if (data_message.data().action == nfs::DataMessage::Action::kDelete) {
    HandleDelete<Data>(data_message);
  } else if (data_message.data().action == nfs::DataMessage::Action::kGet) {
    HandleGet<Data>(data_message, reply_functor);
  }
  AddToAccumulator(data_message);
}
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
template<typename Data>
void StructuredDataManagerService::HandlePut(const nfs::DataMessage& data_message,
                                         const routing::ReplyFunctor& reply_functor) {
  maidsafe_error return_code(CommonErrors::success);
  try {
    Data data(typename Data::name_type(data_message.data().name),
              typename Data::serialised_type(data_message.data().content));
    auto account_name(detail::GetMaidAccountName(data_message));

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

template<typename Data>
void StructuredDataManagerService::HandleDelete(const nfs::DataMessage& data_message,
                                            const routing::ReplyFunctor& reply_functor) {
  try {
    auto data_name(detail::GetDataName<Data>(data_message));
    //DeleteFromAccount<Data>(detail::GetSourceMaidName(data_message), data_name, version);
  }
  catch(const maidsafe_error& error) {
    LOG(kWarning) << error.what();
  }
  catch(...) {
    LOG(kWarning) << "Unknown error.";
   }
}

template<typename Data, nfs::MessageAction action>
void StructuredDataManagerService::AddLocalUnresolvedEntryThenSync(const nfs::DataMessage& data_message,
                                                               int32_t cost) {
  auto account_name(detail::GetMaidAccountName(data_message));
  auto unresolved_entry(detail::CreateUnresolvedEntry<Data, action>(data_message, cost,
                                                                    routing_.kNodeId()));
  maid_account_handler_.AddLocalUnresolvedEntry(account_name, unresolved_entry);
  Sync(account_name);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_MANAGER_SERVICE_H_
