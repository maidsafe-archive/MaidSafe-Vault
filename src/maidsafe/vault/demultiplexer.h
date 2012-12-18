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

#include <functional>
#include <string>

#include"maidsafe/routing/api_config.h"

namespace maidsafe {

namespace nfs {
class Message;
}

namespace vault {

class MaidAccountHolder;
class MetadataManager;
class PmidAccountHolder;
class DataHolder;

class Demultiplexer {
 public:
  Demultiplexer(MaidAccountHolder& maid_account_holder,
                MetadataManager& metadata_manager,
                PmidAccountHolder& pmid_account_holder,
                DataHolder& data_holder);
  void HandleMessage(const std::string& serialised_message, routing::ReplyFunctor reply_functor);
  bool HaveCache(std::string& serialised_message);
  void StoreCache(const std::string& serialised_message);

 private:
  void HandleMessageType(const nfs::Message& message, routing::ReplyFunctor reply_functor);
  bool HandleHaveCache(nfs::Message &message);
  void HandleStoreCache(const nfs::Message& message);

  MaidAccountHolder& maid_account_holder_;
  MetadataManager& metadata_manager_;
  PmidAccountHolder& pmid_account_holder_;
  DataHolder& data_holder_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DEMULTIPLEXER_H_
