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


namespace maidsafe {

namespace vault {

class MaidAccountHolder;
class MetadataManagerService;
class PmidAccountHolder;
class DataHolder;

class Demultiplexer {
 public:
  Demultiplexer(MaidAccountHolder& maid_account_holder,
                MetadataManagerService& metadata_manager_service,
                PmidAccountHolder& pmid_account_holder,
                DataHolder& data_holder);
  void HandleMessage(const std::string& serialised_message,
                     const routing::ReplyFunctor& reply_functor);
  bool GetFromCache(std::string& serialised_message);
  void StoreInCache(const std::string& serialised_message);

 private:
  void HandleDataMessagePersona(nfs::DataMessage& data_message,
                                const routing::ReplyFunctor& reply_functor);
  void HandleGenericMessagePersona(nfs::GenericMessage& generic_message,
                                   const routing::ReplyFunctor& reply_functor);
  NonEmptyString HandleGetFromCache(nfs::DataMessage& data_message);
  void HandleStoreInCache(const nfs::DataMessage& data_message);

  MaidAccountHolder& maid_account_holder_;
  MetadataManagerService& metadata_manager_service_;
  PmidAccountHolder& pmid_account_holder_;
  DataHolder& data_holder_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DEMULTIPLEXER_H_
