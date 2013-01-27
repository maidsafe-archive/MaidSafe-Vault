/***************************************************************************************************
 *  Copyright 2012 PmidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of PmidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of PmidSafe.net.                                  *
 **************************************************************************************************/

#ifndef MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_ACCOUNT_HOLDER_H_
#define MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_ACCOUNT_HOLDER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "boost/filesystem/path.hpp"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/data_message.h"
#include "maidsafe/nfs/generic_message.h"
#include "maidsafe/nfs/message.h"
// #include "maidsafe/nfs/network_file_system.h"
// #include "maidsafe/vault/disk_based_storage.h"
#include "maidsafe/common/rsa.h"


namespace maidsafe {

namespace vault {

class PmidAccountHolder {
  public:
  PmidAccountHolderService(const passport::Pmid& pmid,
                           routing::Routing& routing,
                           nfs::PublicKeyGetter& public_key_getter,
                           const boost::filesystem::path& vault_root_dir);
  template<typename Data>
  void HandleDataMessage(const nfs::DataMessage& data_message,
                         const routing::ReplyFunctor& reply_functor);
  void HandleGenericMessage(const nfs::GenericMessage& generic_message,
                            const routing::ReplyFunctor& reply_functor);
  void HandleSynchronise(const std::vector<routing::NodeInfo>& new_close_nodes);

 private:
  template<typename Data>
  void HandlePut(const nfs::DataMessage& data_message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void HandleDelete(const nfs::DataMessage& data_message,
                    const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void ValidateDataMessage(const nfs::DataMessage& data_message);
  template<typename Data>
  void AdjustAccount(const nfs::DataMessage& data_message);
  template<typename Data>
  void SendDataMessage(const nfs::DataMessage& data_message);

  bool HandleReceivedSyncData(const NonEmptyString& serialised_account);

  routing::Routing& routing_;
  nfs::PublicKeyGetter& public_key_getter_;
  nfs::Accumulator accumulator_;
  PmidAccountHandler pmid_account_handler_;
  PmidAccountHolderNfs nfs_;
};



}  // namespace vault

}  // namespace maidsafe


#endif  // MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_ACCOUNT_HOLDER_H_
