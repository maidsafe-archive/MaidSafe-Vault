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

#ifndef MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_SERVICE_H_
#define MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_SERVICE_H_

#include <type_traits>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/types.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/accumulator.h"
#include "maidsafe/nfs/data_message.h"
#include "maidsafe/nfs/generic_message.h"
#include "maidsafe/nfs/public_key_getter.h"

#include "maidsafe/vault/maid_account_handler.h"
#include "maidsafe/vault/sync_pb.h"
#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

namespace protobuf { class MaidAccountSyncResponse; }

class MaidAccountHolderService {
 public:
  MaidAccountHolderService(const passport::Pmid& pmid,
                           routing::Routing& routing,
                           nfs::PublicKeyGetter& public_key_getter,
                           const boost::filesystem::path& vault_root_dir);
  template<typename Data>
  void HandleDataMessage(const nfs::DataMessage& data_message,
                         const routing::ReplyFunctor& reply_functor);
  void HandleGenericMessage(const nfs::GenericMessage& generic_message,
                            const routing::ReplyFunctor& reply_functor);
  void TriggerSync();

 private:
  MaidAccountHolderService(const MaidAccountHolderService&);
  MaidAccountHolderService& operator=(const MaidAccountHolderService&);
  MaidAccountHolderService(MaidAccountHolderService&&);
  MaidAccountHolderService& operator=(MaidAccountHolderService&&);

  struct SharedResponse {
    SharedResponse();

    mutable std::mutex mutex;
    int count;
    bool this_node_in_group;
  };

  template<typename Data>
  void HandlePut(const nfs::DataMessage& data_message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void HandleDelete(const nfs::DataMessage& data_message,
                    const routing::ReplyFunctor& reply_functor);
  void ValidateDataMessage(const nfs::DataMessage& data_message) const;
  template<typename Data>
  void AdjustAccount(const nfs::DataMessage& data_message,
                     std::true_type,
                     int32_t replication_count = 1);
  // no-op for non-payable data
  template<typename Data>
  void AdjustAccount(const nfs::DataMessage& data_message, std::false_type) {}
  template<typename Data>
  void SendDataMessage(const nfs::DataMessage& data_message);
  void HandleSyncMessage(const nfs::GenericMessage& generic_message,
                         const routing::ReplyFunctor& reply_functor);
  void SendSyncData(const MaidName& account_name);
  bool HandleReceivedSyncData(const NonEmptyString& serialised_account);
//   bool HandleNewComer(const passport::/*PublicMaid*/PublicPmid& p_maid);
//   bool OnKeyFetched(const passport::/*PublicMaid*/PublicPmid& p_maid,
//                     const passport::PublicPmid& p_pmid);
//  bool HandleNewComer(const nfs::PmidRegistration& pmid_registration);
//  void OnGenericErrorHandler(nfs::GenericMessage generic_message);
  void HandleFileRequest(const MaidName& account_name,
                         const protobuf::GetArchiveFiles& requested_files);
  routing::Routing& routing_;
  nfs::PublicKeyGetter& public_key_getter_;
  nfs::Accumulator<MaidName> accumulator_;
  MaidAccountHandler maid_account_handler_;
  MaidAccountHolderNfs nfs_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/maid_account_holder_service-inl.h"

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_SERVICE_H_
