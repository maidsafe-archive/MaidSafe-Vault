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

#ifndef MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HOLDER_SERVICE_H_
#define MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HOLDER_SERVICE_H_

#include <mutex>
#include <string>
#include <type_traits>
#include <vector>

#include "maidsafe/common/types.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/data_message.h"
#include "maidsafe/nfs/generic_message.h"
#include "maidsafe/nfs/public_key_getter.h"

#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/db.h"
#include "maidsafe/vault/sync.pb.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/maid_account_holder/maid_account_handler.h"


namespace maidsafe {

namespace vault {

//   TODO(dirvine / Fraser) where does this go man !!
//    protobuf::MaidAccount proto_maid_account;
//    if (!proto_maid_account.ParseFromString(serialised_account->string()))
//      ThrowError(CommonErrors::parsing_error);


namespace protobuf { class MaidAccountSyncResponse; }

class MaidAccountHolderService {
 public:
  MaidAccountHolderService(const passport::Pmid& pmid,
                           routing::Routing& routing,
                           nfs::PublicKeyGetter& public_key_getter,
                           Db& db);
  template<typename Data>
  void HandleDataMessage(const nfs::DataMessage& data_message,
                         const routing::ReplyFunctor& reply_functor);
  void HandleGenericMessage(const nfs::GenericMessage& generic_message,
                            const routing::ReplyFunctor& reply_functor);
  static int DefaultPaymentFactor() { return kDefaultPaymentFactor_; }
  void HandleChurnEvent(const NodeId& old_node, const NodeId& new_node);

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

  struct PmidRegistrationOp {
    PmidRegistrationOp(const nfs::PmidRegistration& pmid_registration_in,
                       const routing::ReplyFunctor& reply_functor_in)
        : pmid_registration(pmid_registration_in),
          reply_functor(reply_functor_in),
          public_maid(),
          public_pmid(),
          count(0),
          mutex() {}
    template<typename PublicFobType>
    void SetPublicFob(std::unique_ptr<PublicFobType>&&);
    nfs::PmidRegistration pmid_registration;
    routing::ReplyFunctor reply_functor;
    std::unique_ptr<passport::PublicMaid> public_maid;
    std::unique_ptr<passport::PublicPmid> public_pmid;
    int count;
    std::mutex mutex;
  };

  template<typename Data>
  void HandlePut(const nfs::DataMessage& data_message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void HandleDelete(const nfs::DataMessage& data_message,
                    const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  typename Data::name_type GetDataName(const nfs::DataMessage& data_message) const;
  void ValidateSender(const nfs::DataMessage& data_message) const;
  typedef std::true_type UniqueDataType;
  template<typename Data>
  void SendEarlySuccessReply(const nfs::DataMessage& /*data_message*/,
                             const routing::ReplyFunctor& /*reply_functor*/,
                             bool /*low_space*/,
                             UniqueDataType) {}
  typedef std::false_type NonUniqueDataType;
  template<typename Data>
  void SendEarlySuccessReply(const nfs::DataMessage& data_message,
                             const routing::ReplyFunctor& reply_functor,
                             bool low_space,
                             NonUniqueDataType);

  template<typename Data>
  void HandlePutResult(const nfs::Reply& overall_result,
                       const MaidName& account_name,
                       const typename Data::name_type& data_name,
                       routing::ReplyFunctor client_reply_functor,
                       bool low_space,
                       UniqueDataType);

  template<typename Data>
  void HandlePutResult(const nfs::Reply& overall_result,
                       const MaidName& account_name,
                       const typename Data::name_type& data_name,
                       routing::ReplyFunctor client_reply_functor,
                       bool low_space,
                       NonUniqueDataType);

  // ================ Pmid registration ================
  void HandleRegisterPmid(const nfs::GenericMessage& generic_message,
                          const routing::ReplyFunctor& reply_functor);
  template<typename PublicFobType>
  void ValidateRegisterPmid(const nfs::Reply& reply,
                            typename PublicFobType::name_type public_fob_name,
                            std::shared_ptr<PmidRegistrationOp> pmid_registration_op);
  void FinaliseRegisterPmid(std::shared_ptr<PmidRegistrationOp> pmid_registration_op);
  bool DoRegisterPmid(std::shared_ptr<PmidRegistrationOp> pmid_registration_op);

  // ================ Periodic sync ================
  void SendSyncData(const MaidName& account_name);
  void HandleSendSyncDataCallback(const std::string& response,
                                  const MaidName& account_name,
                                  std::shared_ptr<SharedResponse> shared_response);

  void HandleSyncMessage(const nfs::GenericMessage& generic_message,
                         const routing::ReplyFunctor& reply_functor);
  void HandleReceivedSyncInfo(const NonEmptyString& serialised_account,
                              const routing::ReplyFunctor& reply_functor);
  void UpdatePmidTotals(const MaidName& account_name);
  void UpdatePmidTotalsCallback(const std::string& response,
                                const MaidName& account_name,
                                std::shared_ptr<SharedResponse> shared_response);

  // ================ Account transfer ================
  void HandleAccountTransfer(const nfs::GenericMessage& generic_message,
                             const routing::ReplyFunctor& reply_functor);
  void TransferAccount(const MaidName& account_name, const NodeId& new_node);

  routing::Routing& routing_;
  nfs::PublicKeyGetter& public_key_getter_;
  std::mutex accumulator_mutex_;
  Accumulator<MaidName> accumulator_;
  MaidAccountHandler maid_account_handler_;
  MaidAccountHolderNfs nfs_;
  static const int kPutRepliesSuccessesRequired_;
  static const int kDefaultPaymentFactor_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/maid_account_holder/maid_account_holder_service-inl.h"

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_HOLDER_SERVICE_H_
