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

struct PmidRegistrationOp;
struct GetPmidTotalsOp;

class MaidAccountHolderService {
 public:
  MaidAccountHolderService(const passport::Pmid& pmid,
                           routing::Routing& routing,
                           nfs::PublicKeyGetter& public_key_getter,
                           Db& db);
  // Handling of received requests (sending of requests is done via nfs_ object).
  template<typename Data>
  void HandleDataMessage(const nfs::DataMessage& data_message,
                         const routing::ReplyFunctor& reply_functor);
  void HandleGenericMessage(const nfs::GenericMessage& generic_message,
                            const routing::ReplyFunctor& reply_functor);
  void HandleChurnEvent(routing::MatrixChange matrix_change);
  static int DefaultPaymentFactor() { return kDefaultPaymentFactor_; }

 private:
  MaidAccountHolderService(const MaidAccountHolderService&);
  MaidAccountHolderService& operator=(const MaidAccountHolderService&);
  MaidAccountHolderService(MaidAccountHolderService&&);
  MaidAccountHolderService& operator=(MaidAccountHolderService&&);

  void ValidateSender(const nfs::DataMessage& data_message) const;
  void ValidateSender(const nfs::GenericMessage& generic_message) const;

  // =============== Put/Delete data ===============================================================
  template<typename Data>
  void HandlePut(const nfs::DataMessage& data_message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void HandleDelete(const nfs::DataMessage& data_message,
                    const routing::ReplyFunctor& reply_functor);
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
  void SendReplyAndAddToAccumulator(const nfs::DataMessage& data_message,
                                    const routing::ReplyFunctor& reply_functor,
                                    const nfs::Reply& reply);

  template<typename Data>
  void HandlePutResult(const nfs::Reply& overall_result,
                       const nfs::DataMessage& data_message,
                       routing::ReplyFunctor client_reply_functor,
                       bool low_space,
                       UniqueDataType);

  template<typename Data>
  void HandlePutResult(const nfs::Reply& overall_result,
                       const nfs::DataMessage& data_message,
                       routing::ReplyFunctor client_reply_functor,
                       bool low_space,
                       NonUniqueDataType);

  // =============== Pmid registration =============================================================
  void HandlePmidRegistration(const nfs::GenericMessage& generic_message,
                              const routing::ReplyFunctor& reply_functor);
  template<typename PublicFobType>
  void ValidatePmidRegistration(const nfs::Reply& reply,
                                typename PublicFobType::name_type public_fob_name,
                                std::shared_ptr<PmidRegistrationOp> pmid_registration_op);
  void FinalisePmidRegistration(std::shared_ptr<PmidRegistrationOp> pmid_registration_op);

  // =============== Sync ==========================================================================
  void Sync(const MaidName& account_name);
  void HandleSync(const nfs::GenericMessage& generic_message);

  // =============== Account transfer ==============================================================
  void TransferAccount(const MaidName& account_name, const NodeId& new_node);
  void HandleAccountTransfer(const nfs::GenericMessage& generic_message);

  // =============== PMID totals ===================================================================
  void UpdatePmidTotals(const MaidName& account_name);
  void UpdatePmidTotalsCallback(const std::string& serialised_reply,
                                std::shared_ptr<GetPmidTotalsOp> op_data);

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
