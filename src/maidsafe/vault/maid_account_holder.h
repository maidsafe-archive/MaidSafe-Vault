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

#ifndef MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_H_
#define MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <fstream>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "maidsafe/routing/api_config.h"

#include "maidsafe/nfs/maid_account.h"
#include "maidsafe/nfs/message.h"
#include "maidsafe/nfs/public_key_getter.h"
#include "maidsafe/nfs/nfs.h"


namespace maidsafe {

namespace vault {

class MaidAccountHolder {
 public:
  MaidAccountHolder(const passport::Pmid& pmid, routing::Routing& routing,
                    const boost::filesystem::path& vault_root_dir);
  ~MaidAccountHolder();
  template<typename Data>
  void HandleMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  void OnCloseNodeReplaced(const std::vector<routing::NodeInfo>& new_close_nodes);
  void Serialise();
  void Serialise(const passport::Maid& maid);
  void Serialise(const passport::Pmid& pmid);
  void RemoveAccount(const passport::Maid& maid);

 private:
  template<typename Data>
  void HandleGetMessage(nfs::Message message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void HandlePutMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void HandlePostMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void HandleDeleteMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  void SendSyncData();
  bool HandleNewComer(const passport::PublicMaid& p_maid);

  routing::Routing& routing_;
  const boost::filesystem::path kRootDir_;
  nfs::MaidAccountHolderNfs nfs_;
  std::vector<maidsafe::nfs::MaidAccount> maid_accounts_;
  nfs::PublicKeyGetter public_key_getter_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/maid_account_holder-inl.h"

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_H_
