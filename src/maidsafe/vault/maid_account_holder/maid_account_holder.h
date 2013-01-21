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

#ifndef MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HOLDER_H_
#define MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HOLDER_H_

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <fstream>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "maidsafe/routing/api_config.h"

#include "maidsafe/nfs/message.h"
#include "maidsafe/nfs/pmid_registration.h"
#include "maidsafe/nfs/generic_message.h"
#include "maidsafe/nfs/public_key_getter.h"

#include "maidsafe/vault/types.h"
#include "maidsafe/vault/maid_account_holder/maid_account_handler.h"

namespace maidsafe {

namespace vault {

class MaidAccount;

class MaidAccountHolder {
 public:
  MaidAccountHolder(const passport::Pmid& pmid,
                    routing::Routing& routing,
                    nfs::PublicKeyGetter& public_key_getter,
                    const boost::filesystem::path& vault_root_dir);
  ~MaidAccountHolder();
  template<typename Data>
  void HandleDataMessage(const nfs::DataMessage& data_message,
                         const routing::ReplyFunctor& reply_functor);
  void HandleGenericMessage(const nfs::GenericMessage& generic_message,
                            const routing::ReplyFunctor& reply_functor);
  void CloseNodeReplaced(const std::vector<routing::NodeInfo>& new_close_nodes) {
    (void)new_close_nodes;
  }
  void Serialise() const;
  void Serialise(const passport::Maid& maid) const;
  void Serialise(const passport::Pmid& pmid) const;
  void RemoveAccount(const passport::Maid& maid);

 private:
  template<typename Data>
  void HandleGetMessage(const nfs::DataMessage& data_message,
                        const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void HandlePutMessage(const nfs::DataMessage& data_message,
                        const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void HandleDeleteMessage(const nfs::DataMessage& data_message,
                           const routing::ReplyFunctor& reply_functor);
  // Handles payable data type(s)
  template<typename Data>
  void AdjustAccount(const nfs::DataMessage& data_message,
                     const routing::ReplyFunctor& reply_functor,
                     std::true_type);
  // no-op for non-payable data
  template<typename Data>
  void AdjustAccount(const nfs::DataMessage& /*data_message*/,
                     const routing::ReplyFunctor& /*reply_functor*/,
                     std::false_type) {}
  void SendSyncData();
  bool HandleReceivedSyncData(const NonEmptyString& serialised_account);
//   bool HandleNewComer(const passport::/*PublicMaid*/PublicPmid& p_maid);
//   bool OnKeyFetched(const passport::/*PublicMaid*/PublicPmid& p_maid,
//                     const passport::PublicPmid& p_pmid);
  bool HandleNewComer(const nfs::PmidRegistration& pmid_registration);

  // On error handler
  template<typename Data>
  void OnPutErrorHandler(nfs::DataMessage data_message);
  template<typename Data>
  void OnDeleteErrorHandler(nfs::DataMessage data_message);
  void OnPostErrorHandler(nfs::GenericMessage generic_message);

  routing::Routing& routing_;
  const boost::filesystem::path kRootDir_;
  MaidAccountHolderNfs nfs_;
  nfs::PublicKeyGetter& public_key_getter_;
//  std::vector<MaidAccount> maid_accounts_;
  MaidAccountHandler maid_account_handler_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/maid_account_holder/maid_account_holder-inl.h"

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HOLDER_H_
