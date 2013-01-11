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

#ifndef MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_MANAGER_INL_H_
#define MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_MANAGER_INL_H_

#include <exception>
#include <string>

#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

template<typename Data>
void MetadataManager::HandlePutMessage(const nfs::Message& message,
                                       const routing::ReplyFunctor& reply_functor) {
//  if (request_queue_.Push(message.id(), message.name()))
//    nfs_.Put<Data>(message, reply_functor);
  reply_functor(nfs::ReturnCode(0).Serialise()->string());
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_MANAGER_INL_H_
