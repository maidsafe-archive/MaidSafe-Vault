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

template<typename Data>
void StructuredDataManagerService::HandleMessage(const nfs::Message& message,
                                                  const routing::ReplyFunctor& reply_functor) {
 //   ValidateSender(message);

  nfs::Reply reply(CommonErrors::pending_result);
  {
    std::lock_guard<std::mutex> lock(accumulator_mutex_);
    if (accumulator_.CheckHandled(message, reply))
      return reply_functor(reply.Serialise()->string());
  }

   if (message.data().action == nfs::MessageAction::kSynchronise)
     return HandleSync(message);   // No accumulate
   if (message.data().action == nfs::MessageAction::kAccountTransfer)
     return HandleAccountTransfer(message);   // No accumulate
   if (message.data().action == nfs::MessageAction::kGet)
     return HandleGet(message, reply_functor);  //  Add to accumulator on action
   if (message.data().action == nfs::MessageAction::kGetBranch)
     return HandleGetBranch(message, reply_functor);  //  Add to accumulator on action

   if (message.data().action != nfs::MessageAction::kPut &&
       message.data().action != nfs::MessageAction::kDeleteBranchUntilFork &&
       message.data().action != nfs::MessageAction::kDelete)
     ThrowError(CommonErrors::invalid_parameter);

   // accumulate then action, on completion then set reply
   std::lock_guard<std::mutex> lock(accumulator_mutex_);
   if (!accumulator_.PushSingleResult(message,
                                     reply_functor,
                                     maidsafe_error(CommonErrors::pending_result)).size() <
                                     routing::Parameters::node_group_size -1) {
     Sync<Data>(message);
   }
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_MANAGER_SERVICE_INL_H_
