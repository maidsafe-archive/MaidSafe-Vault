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

#include "maidsafe/common/rsa.h"

#include "maidsafe/routing/routing_api.h"

#include "maidsafe/nfs/data_message.h"
#include "maidsafe/nfs/generic_message.h"
#include "maidsafe/nfs/message.h"

#include "maidsafe/vault/pmid_account_handler.h"
#include "maidsafe/vault/types.h"

namespace maidsafe {

namespace vault {

class PmidAccountHolder {
  public:
  PmidAccountHolder(const passport::Pmid& pmid,
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

  void ValidateDataMessage(const nfs::DataMessage& data_message);
  void InformOfDataHolderDown(const PmidName& pmid_name);
  void InformOfDataHolderUp(const PmidName& pmid_name);
  void InformAboutDataHolder(const PmidName& pmid_name, nfs::GenericMessage::Action action);
  void GetDataNamesInAccount(const PmidName& pmid_name, std::vector<PmidName>& metada_manager_ids);
  void SendSyncMessage(const PmidName& pmid_name, nfs::GenericMessage::Action action);

  routing::Routing& routing_;
  nfs::PublicKeyGetter& public_key_getter_;
  nfs::Accumulator accumulator_;
  PmidAccountHandler pmid_account_handler_;
  PmidAccountHolderNfs nfs_;
};



}  // namespace vault

}  // namespace maidsafe


#endif  // MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_ACCOUNT_HOLDER_H_
