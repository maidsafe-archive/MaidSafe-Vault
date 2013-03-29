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

#ifndef MAIDSAFE_VAULT_DEMULTIPLEXER_H_
#define MAIDSAFE_VAULT_DEMULTIPLEXER_H_

#include <string>

#include "maidsafe/common/types.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/nfs.h"
#include "maidsafe/nfs/generic_message.h"


namespace maidsafe {

namespace vault {

class MaidAccountHolderService;
class MetadataManagerService;
class PmidAccountHolderService;
class DataHolderService;

class Demultiplexer {
 public:
  Demultiplexer(MaidAccountHolderService& maid_account_holder_service,
                MetadataManagerService& metadata_manager_service,
                PmidAccountHolderService& pmid_account_holder_service,
                DataHolderService& data_holder);
  void HandleMessage(const std::string& serialised_message,
                     const routing::ReplyFunctor& reply_functor);
  bool GetFromCache(std::string& serialised_message);
  void StoreInCache(const std::string& serialised_message);

 private:
  template<typename MessageType>
  void PersonaHandleMessage(const MessageType& message,
                            const routing::ReplyFunctor& reply_functor);
  NonEmptyString HandleGetFromCache(const nfs::DataMessage& data_message);
  void HandleStoreInCache(const nfs::DataMessage& data_message);

  MaidAccountHolderService& maid_account_holder_service_;
  MetadataManagerService& metadata_manager_service_;
  PmidAccountHolderService& pmid_account_holder_service_;
  DataHolderService& data_holder_;
};

template<>
void Demultiplexer::PersonaHandleMessage<nfs::DataMessage>(
    const nfs::DataMessage& message,
    const routing::ReplyFunctor& reply_functor);

template<>
void Demultiplexer::PersonaHandleMessage<nfs::GenericMessage>(
    const nfs::GenericMessage& message,
    const routing::ReplyFunctor& reply_functor);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DEMULTIPLEXER_H_
