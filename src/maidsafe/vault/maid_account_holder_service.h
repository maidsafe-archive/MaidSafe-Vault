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

#ifndef MAIDSAFE_VAULT_SERVICE_H_
#define MAIDSAFE_VAULT_SERVICE_H_

#include <type_traits>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/types.h"

#include "maidsafe/passport/types.h"

#include "maidsafe/routing/api_config.h"

#include "maidsafe/nfs/data_message.h"
#include "maidsafe/nfs/generic_message.h"
#include "maidsafe/nfs/public_key_getter.h"

#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

template<typename Account>
class AccountHandler;

template<typename Account, typename Nfs>
class Service {
 public:
  Service(const passport::Pmid& pmid,
          routing::Routing& routing,
          nfs::PublicKeyGetter& public_key_getter,
          const boost::filesystem::path& vault_root_dir);
  ~Service();
  template<typename Data>
  void HandleDataMessage(const nfs::DataMessage& data_message,
                         const routing::ReplyFunctor& reply_functor);
  void HandleGenericMessage(const nfs::GenericMessage& generic_message,
                            const routing::ReplyFunctor& reply_functor);
  void HandleSynchronise(const std::vector<routing::NodeInfo>& new_close_nodes);

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
  template<typename Data>
  void AdjustAccount(const nfs::DataMessage& data_message,
                     const routing::ReplyFunctor& reply_functor,
                     std::true_type);
  template<typename Data>
  void AdjustAccount(const nfs::DataMessage& data_message,
                     const routing::ReplyFunctor& reply_functor,
                     std::false_type);
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
  void OnGenericErrorHandler(nfs::GenericMessage generic_message);

  routing::Routing& routing_;
  const boost::filesystem::path kRootDir_;
  Nfs nfs_;
  nfs::PublicKeyGetter& public_key_getter_;
  AccountHandler<Account> account_handler_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/service-inl.h"

#endif  // MAIDSAFE_VAULT_SERVICE_H_
