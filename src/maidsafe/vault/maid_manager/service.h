/*  Copyright 2012 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.maidsafe.net/licenses

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_SERVICE_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_SERVICE_H_

#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <vector>

#include "boost/mpl/vector.hpp"
#include "boost/mpl/insert_range.hpp"
#include "boost/mpl/end.hpp"

#include "maidsafe/common/error.h"
#include "maidsafe/common/log.h"
#include "maidsafe/common/on_scope_exit.h"
#include "maidsafe/common/types.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/routing/routing_api.h"

#include "maidsafe/nfs/message_types.h"
#include "maidsafe/nfs/message_wrapper.h"
#include "maidsafe/nfs/utils.h"
#include "maidsafe/nfs/client/data_getter.h"

#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/group_db.h"
#include "maidsafe/vault/message_types.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/unresolved_action.h"
#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/maid_manager/action_create_remove_account.h"
#include "maidsafe/vault/maid_manager/action_put.h"
#include "maidsafe/vault/maid_manager/action_delete.h"
#include "maidsafe/vault/maid_manager/action_register_unregister_pmid.h"
#include "maidsafe/vault/maid_manager/dispatcher.h"
#include "maidsafe/vault/maid_manager/maid_manager.h"
#include "maidsafe/vault/maid_manager/metadata.h"
#include "maidsafe/vault/maid_manager/maid_manager.pb.h"
#include "maidsafe/vault/sync.h"
#include "maidsafe/vault/accumulator.h"

namespace maidsafe {

class OwnerDirectory;
class GroupDirectory;
class WorldDirectory;

namespace vault {

class AccountDb;
struct PmidRegistrationOp;
struct GetPmidTotalsOp;

class MaidManagerService {
 public:
  typedef nfs::MaidManagerServiceMessages PublicMessages;
  typedef MaidManagerServiceMessages VaultMessages;

  MaidManagerService(const passport::Pmid& pmid, routing::Routing& routing);

  template <typename T>
  void HandleMessage(const T&, const typename T::Sender&, const typename T::Receiver&);
  void HandleChurnEvent(std::shared_ptr<routing::MatrixChange> /*matrix_change*/) {}

 private:
  static int DefaultPaymentFactor() { return kDefaultPaymentFactor_; }

 private:
  MaidManagerService(const MaidManagerService&);
  MaidManagerService& operator=(const MaidManagerService&);
  MaidManagerService(MaidManagerService&&);
  MaidManagerService& operator=(MaidManagerService&&);

  //  void CheckSenderIsConnectedMaidNode(const nfs::Message& message) const;
  //  void CheckSenderIsConnectedMaidManager(const nfs::Message& message) const;
  //  void ValidateDataSender(const nfs::Message& message) const;
  template <typename MessageType>
  bool ValidateSender(const MessageType& /*message*/,
                      const typename MessageType::Sender& /*sender*/) const {
    return false;
  }

  // =============== account creation ==============================================================
  void HandleCreateMaidAccount(const passport::PublicMaid &maid,
                               const passport::PublicAnmaid& anmaid,
                               const nfs::MessageId& message_id);

  // =============== Put/Delete data ===============================================================
  template <typename Data>
  void HandlePut(const MaidName& account_name, const Data& data, const PmidName& pmid_node_hint,
                 const nfs::MessageId& message_id);

  template <typename Data>
  void HandlePutResponse(const MaidName& maid_name, const typename Data::Name& data_name,
                         const int32_t& cost, const nfs::MessageId& message_id);

  template <typename Data>
  void HandlePutFailure(const MaidName& maid_name, const typename Data::Name& data_name,
                        const maidsafe_error& error, const nfs::MessageId& message_id);

  template <typename Data>
  void HandleDelete(const MaidName& account_name, const typename Data::Name& data_name,
                    const nfs::MessageId& message_id);

  template <typename Data>
  bool DeleteAllowed(const MaidName& account_name, const typename Data::Name& data_name);

  MaidManagerMetadata::Status AllowPut(const MaidName& account_name, int32_t cost);

  // Only Maid and Anmaid can create account; for all others this is a no-op.
  typedef std::true_type AllowedAccountCreationType;
  typedef std::false_type DisallowedAccountCreationType;
  template <typename Data>
  void CreateAccount(const MaidName& account_name, AllowedAccountCreationType);
  template <typename Data>
  void CreateAccount(const MaidName& /*account_name*/, DisallowedAccountCreationType) {}
  void FinalisePmidRegistration(std::shared_ptr<PmidRegistrationOp> pmid_registration_op);


  // ===================================== PMID totals ============================================
  void UpdatePmidTotals(const MaidName& account_name);
  void UpdatePmidTotalsCallback(const std::string& serialised_reply,
                                std::shared_ptr<GetPmidTotalsOp> op_data);

  void DoSync();

  typedef boost::mpl::vector<> InitialType;
  typedef boost::mpl::insert_range<InitialType,
                                   boost::mpl::end<InitialType>::type,
                                   nfs::MaidManagerServiceMessages::types>::type IntermediateType;
  typedef boost::mpl::insert_range<IntermediateType,
                                   boost::mpl::end<IntermediateType>::type,
                                   MaidManagerServiceMessages::types>::type FinalType;
 public:
  typedef boost::make_variant_over<FinalType>::type Messages;

 private:
  routing::Routing& routing_;
  //  nfs_client::DataGetter data_getter_;
  GroupDb<MaidManager> group_db_;
  std::mutex accumulator_mutex_;
  Accumulator<Messages> accumulator_;
  MaidManagerDispatcher dispatcher_;
  Sync<MaidManager::UnresolvedCreateAccount> sync_create_accounts_;
  Sync<MaidManager::UnresolvedRemoveAccount> sync_remove_accounts_;
  Sync<MaidManager::UnresolvedPut> sync_puts_;
  Sync<MaidManager::UnresolvedDelete> sync_deletes_;
  Sync<MaidManager::UnresolvedRegisterPmid> sync_register_pmids_;
  Sync<MaidManager::UnresolvedUnregisterPmid> sync_unregister_pmids_;
  static const int kDefaultPaymentFactor_;
};

template <typename T>
void MaidManagerService::HandleMessage(const T&, const typename T::Sender&,
                                       const typename T::Receiver&) {
  //  T::specialisation_required;
}

template <>
void MaidManagerService::HandleMessage(
    const nfs::PutRequestFromMaidNodeToMaidManager& message,
    const typename nfs::PutRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::PutRequestFromMaidNodeToMaidManager::Receiver& receiver);

template <>
void MaidManagerService::HandleMessage(
    const PutResponseFromDataManagerToMaidManager& message,
    const typename PutResponseFromDataManagerToMaidManager::Sender& sender,
    const typename PutResponseFromDataManagerToMaidManager::Receiver& receiver);

template <>
void MaidManagerService::HandleMessage(
    const PutFailureFromDataManagerToMaidManager& message,
    const typename PutFailureFromDataManagerToMaidManager::Sender& sender,
    const typename PutFailureFromDataManagerToMaidManager::Receiver& receiver);


template <>
void MaidManagerService::HandleMessage(
    const nfs::DeleteRequestFromMaidNodeToMaidManager& message,
    const typename nfs::DeleteRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::DeleteRequestFromMaidNodeToMaidManager::Receiver& receiver);

template <>
void MaidManagerService::HandleMessage(
    const nfs::PutVersionRequestFromMaidNodeToMaidManager& message,
    const typename nfs::PutVersionRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::PutVersionRequestFromMaidNodeToMaidManager::Receiver& receiver);

template <>
void MaidManagerService::HandleMessage(
    const nfs::DeleteBranchUntilForkRequestFromMaidNodeToMaidManager& message,
    const typename nfs::DeleteBranchUntilForkRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::DeleteBranchUntilForkRequestFromMaidNodeToMaidManager::Receiver& receiver);

template <>
void MaidManagerService::HandleMessage(
    const nfs::CreateAccountRequestFromMaidNodeToMaidManager& message,
    const typename nfs::CreateAccountRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::CreateAccountRequestFromMaidNodeToMaidManager::Receiver& receiver);

template <>
void MaidManagerService::HandleMessage(
    const nfs::RemoveAccountRequestFromMaidNodeToMaidManager& message,
    const typename nfs::RemoveAccountRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::RemoveAccountRequestFromMaidNodeToMaidManager::Receiver& receiver);

template <>
void MaidManagerService::HandleMessage(
    const nfs::RegisterPmidRequestFromMaidNodeToMaidManager& message,
    const typename nfs::RegisterPmidRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::RegisterPmidRequestFromMaidNodeToMaidManager::Receiver& receiver);

template <>
void MaidManagerService::HandleMessage(
    const nfs::UnregisterPmidRequestFromMaidNodeToMaidManager& message,
    const typename nfs::UnregisterPmidRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::UnregisterPmidRequestFromMaidNodeToMaidManager::Receiver& receiver);

template <>
void MaidManagerService::HandleMessage(
    const nfs::GetPmidHealthRequestFromMaidNodeToMaidManager& message,
    const typename nfs::GetPmidHealthRequestFromMaidNodeToMaidManager::Sender& sender,
    const typename nfs::GetPmidHealthRequestFromMaidNodeToMaidManager::Receiver& receiver);

template <>
void MaidManagerService::HandleMessage(
    const SynchroniseFromMaidManagerToMaidManager& message,
    const typename SynchroniseFromMaidManagerToMaidManager::Sender& sender,
    const typename SynchroniseFromMaidManagerToMaidManager::Receiver& receiver);

// ==================== Implementation =============================================================
namespace detail {

template <typename T>
struct can_create_account : public std::false_type {};

template <>
struct can_create_account<passport::PublicAnmaid> : public std::true_type {};

template <>
struct can_create_account<passport::PublicMaid> : public std::true_type {};

// template<typename Data>
// int32_t EstimateCost(const Data& data) {
//  static_assert(!std::is_same<Data, passport::PublicAnmaid>::value, "Cost of Anmaid should be
// 0.");
//  static_assert(!std::is_same<Data, passport::PublicMaid>::value, "Cost of Maid should be 0.");
//  static_assert(!std::is_same<Data, passport::PublicPmid>::value, "Cost of Pmid should be 0.");
//  return static_cast<int32_t>(MaidManagerService::DefaultPaymentFactor() *
//                              data.content.string().size());
//}

// template<>
// int32_t EstimateCost<passport::PublicAnmaid>(const passport::PublicAnmaid&);

// template<>
// int32_t EstimateCost<passport::PublicMaid>(const passport::PublicMaid&);

// template<>
// int32_t EstimateCost<passport::PublicPmid>(const passport::PublicPmid&);

// MaidName GetMaidAccountName(const nfs::Message& message);

 template<typename DataName>
 DataName GetObfuscatedDataName(const DataName& data_name) {
  // Hash the data name to obfuscate the list of chunks associated with the client.
  return DataName(crypto::Hash<crypto::SHA512>(data_name.raw_name));
}

template <typename MaidManagerSyncType>
void IncrementAttemptsAndSendSync(MaidManagerDispatcher& dispatcher,
                                  MaidManagerSyncType& sync_type) {
  auto unresolved_actions(sync_type.GetUnresolvedActions());
  if (!unresolved_actions.empty()) {
    sync_type.IncrementSyncAttempts();
    for (const auto& unresolved_action : unresolved_actions)
      dispatcher.SendSync(unresolved_action->key.group_name, unresolved_action->Serialise());
  }
}

}  // namespace detail

// ================================== Put Implementation ========================================

template <typename Data>
void MaidManagerService::HandlePut(const MaidName& account_name, const Data& data,
                                   const PmidName& pmid_node_hint,
                                   const nfs::MessageId& message_id) {
  auto metadata(group_db_.GetMetadata(account_name));
  if (metadata->AllowPut(data.data().string().size()) != MaidManagerMetadata::Status::kNoSpace) {
    dispatcher_.SendPutRequest(account_name, data, pmid_node_hint, message_id);
  } else {
    dispatcher_.SendPutFailure<Data>(account_name, data.name(),
                                     nfs_client::ReturnCode(CommonErrors::cannot_exceed_limit),
                                     message_id);
  }
}

template <typename Data>
void MaidManagerService::HandlePutResponse(const MaidName& maid_name,
                                           const typename Data::Name& data_name,
                                           const int32_t& cost, const nfs::MessageId& /*message_id*/) {
  typename MaidManager::Key group_key(typename MaidManager::GroupName(maid_name.value),
                                      GetObfuscatedDataName(data_name), data_name.type);
  sync_puts_.AddLocalAction(typename MaidManager::UnresolvedPut(
      group_key, ActionMaidManagerPut(cost), routing_.kNodeId()));
  DoSync();
}

template <typename Data>
void MaidManagerService::HandlePutFailure(
    const MaidName& maid_name, const typename Data::Name& data_name,
    const maidsafe_error& error, const nfs::MessageId& message_id) {
  dispatcher_.SendPutFailure<Data>(maid_name, data_name, error, message_id);
}

// ================================== Delete Implementation =======================================

template <typename Data>
void MaidManagerService::HandleDelete(const MaidName& account_name,
                                      const typename Data::Name& data_name,
                                      const nfs::MessageId& message_id) {
  if (DeleteAllowed(account_name, data_name)) {
    typename MaidManager::Key group_key(typename MaidManager::GroupName(account_name.value),
                                        GetObfuscatedDataName(data_name), data_name.type);
    sync_deletes_.AddLocalAction(typename MaidManager::UnresolvedDelete(
        group_key, ActionMaidManagerDelete(message_id), routing_.kNodeId()));
    DoSync();
  }
}

template <typename Data>
bool MaidManagerService::DeleteAllowed(const MaidName& account_name,
                                       const typename Data::Name& data_name) {
  try {
    if (group_db_.GetValue(MaidManager::Key(account_name, GetObfuscatedDataName(data_name),
                                            Data::Tag::kValue)))
      return true;
  }
  catch (const maidsafe_error& /*error*/) {
    return false;
  }
  return false;
}


// ===============================================================================================


// template<>
// void MaidManagerService::HandleMessage<maid_manager::MaidNodePut>(
//    const maid_manager::MaidNodePut& message,
//    const typename nfs::Sender<maid_manager::MaidNodePut>::type& sender,
//    const typename nfs::Receiver<maid_manager::MaidNodePut>::type& receiver) {
//  try {
//    AddToAccumulator(message);
//    message.contents
//    nfs::DataNameContentAndPmidHint parsed_content(message.serialised_message);
//    auto data_name_variant(GetDataNameVariant(parsed_content.name_and_content.name.type,
//                                              parsed_content.name_and_content.name.raw_name));
//    DataVisitorPut visitor(this, message, sender);
//    boost::apply_visitor(visitor, data_name_variant);
//  }
//  catch(const maidsafe_error& error) {
//    LOG(kWarning) << error.what();
//  }
//  catch(...) {
//    LOG(kWarning) << "Unknown error.";
//  }
//}

// template<>
// void MaidManagerService::HandleMessage<maid_manager::MaidNodeDelete>(
//    const maid_manager::MaidNodeDelete& message,
//    const typename nfs::Sender<maid_manager::MaidNodeDelete>::type& sender,
//    const typename nfs::Receiver<maid_manager::MaidNodeDelete>::type& receiver) {
//  try {
//    AddToAccumulator(message);
//    nfs::DataName parsed_data_name(message.serialised_message);
//    auto data_name_variant(GetDataNameVariant(parsed_data_name.type, parsed_data_name.raw_name));
//    DataVisitorDelete visitor(this, message, sender);
//    boost::apply_visitor(visitor, data_name_variant);
//  }
//  catch(const maidsafe_error& error) {
//    LOG(kWarning) << error.what();
//  }
//  catch(...) {
//    LOG(kWarning) << "Unknown error.";
//  }
//}

// template<typename Data>
// void HandlePut(const typename Data::Name& data_name,
//               const maid_manager::MaidNodePut& message,
//               const typename nfs::Sender<maid_manager::MaidNodePut>::type& sender) {
//  MaidName account_name(Identity(sender->string()));
//  nfs::DataNameContentAndPmidHint parsed_content(message.serialised_message);

//  message. // figure out from metadata if we have space and if low return to client
//      // we need o send the whole data item not only name
//  AddLocalUnresolvedActionThenSync<Data, nfs::MessageAction::kPutRequest>(message, 0);
//  dispatcher_.SendPutRequest(account_name, data_name);
//}

// template<typename Data>
// void MaidManagerService::HandleDelete(
//    const typename Data::Name& data_name,
//    const maid_manager::MaidNodeDelete& message,
//    const typename nfs::Sender<maid_manager::MaidNodeDelete>::type& sender) {
//  MaidName account_name(Identity(sender->string()));
//  AddLocalUnresolvedActionThenSync<Data, nfs::MessageAction::kDelete>(message, 0);
//  dispatcher_.SendDeleteRequest(account_name, data_name);
//}

// template<typename Data>
// void MaidManagerService::HandleMessage(const nfs::Message& message,
//                                       const routing::ReplyFunctor& reply_functor) {
//  ValidateDataSender(message);
//  nfs::Reply reply(CommonErrors::success);
//  {
//    std::lock_guard<std::mutex> lock(accumulator_mutex_);
////    if (accumulator_.CheckHandled(message, reply))  // FIXME
////      return reply_functor(reply.Serialise()->string());
//  }
//  switch (message.data().action) {
//    case nfs::MessageAction::kPut:
//      return HandlePut<Data>(message, reply_functor);
//    case nfs::MessageAction::kDelete:
//      return HandleDelete<Data>(message, reply_functor);
//    case nfs::MessageAction::kGet:        // intentional fallthrough
//    case nfs::MessageAction::kGetBranch:  // intentional fallthrough
//    case nfs::MessageAction::kDeleteBranchUntilFork:
//      return HandleVersionMessage<Data>(message, reply_functor);
//    default:
//      reply = nfs::Reply(VaultErrors::operation_not_supported, message.Serialise().data);
//      SendReplyAndAddToAccumulator(message, reply_functor, reply);
//  }
//}

// template<typename Data>
// void MaidManagerService::HandlePut(const maidsafe::vault::MaidManagerService::Data &message) {
//  maidsafe_error return_code(CommonErrors::success);
//  try {
//    Data data(typename Data::Name(message.data().name),
//              typename Data::serialised_type(message.data().content));
//    auto account_name(detail::GetMaidAccountName(message));
//    auto estimated_cost(detail::EstimateCost(message.data()));
//    auto account_status(AllowPut(account_name, estimated_cost));

//    if (account_status == MaidManagerMetadata::Status::kNoSpace)
//      ThrowError(VaultErrors::not_enough_space);
//    if (account_status == MaidManagerMetadata::Status::kLowSpace)
//      UpdatePmidTotals(account_name);

//    auto put_op(std::make_shared<nfs::OperationOp>(
//        kPutRepliesSuccessesRequired_,
//        [this, message, reply_functor, low_space](nfs::Reply overall_result) {
//            this->HandlePutResult<Data>(overall_result, message, reply_functor);
//        }));

//    nfs_.Put(data,
//             message.pmid_node(),
//             [put_op](std::string serialised_reply) {
//                 nfs::HandleOperationReply(put_op, serialised_reply);
//             });
//  }
//  catch(const maidsafe_error& error) {
//    LOG(kWarning) << error.what();
//    return_code = error;
//  }
//  catch(...) {
//    LOG(kWarning) << "Unknown error.";
//    return_code = MakeError(CommonErrors::unknown);
//  }
//  nfs::Reply reply(return_code, message.Serialise().data);
//  SendReplyAndAddToAccumulator(message, reply_functor, reply);
//}

// template<>
// void MaidManagerService::HandlePut<OwnerDirectory>(const nfs::Message& message,
//                                                   const routing::ReplyFunctor& reply_functor);

// template<>
// void MaidManagerService::HandlePut<GroupDirectory>(const nfs::Message& message,
//                                                   const routing::ReplyFunctor& reply_functor);

// template<>
// void MaidManagerService::HandlePut<WorldDirectory>(const nfs::Message& message,
//                                                   const routing::ReplyFunctor& reply_functor);

// template<typename Data>
// void MaidManagerService::HandleDelete(const nfs::Message& message,
//                                      const routing::ReplyFunctor& reply_functor) {
//  SendReplyAndAddToAccumulator(message, reply_functor, nfs::Reply(CommonErrors::success));
//  try {
//    auto account_name(detail::GetMaidAccountName(message));
//    typename Data::Name data_name(message.data().name);
//    maid_account_handler_.DeleteData<Data>(account_name, data_name);
//    AddLocalUnresolvedActionThenSync<Data, nfs::MessageAction::kDelete>(message, 0);
//    nfs_.Delete<Data>(data_name, [](std::string /*serialised_reply*/) {});
//  }
//  catch(const maidsafe_error& error) {
//    LOG(kWarning) << error.what();
//    // Always return success for Deletes
//  }
//  catch(...) {
//    LOG(kWarning) << "Unknown error.";
//    // Always return success for Deletes
//  }
//}

// MaidManagerMetadata::Status MaidManagerService::AllowPut(const MaidName& account_name,
//                                                         int32_t cost) {
//  auto metadata(group_db_.GetMetadata(account_name));
//  if (metadata)
//    return metadata->AllowPut(cost);
//  // If A/C doesn't exist, disallow any Puts with non-zero cost.  Effectively blocks client from
//  // Putting data until A/C has been successfully created.
//  return cost ? MaidManagerMetadata::Status::kNoSpace : MaidManagerMetadata::Status::kOk;
//}

// template<typename Data>
// void MaidManagerService::HandlePutResult(const nfs::Reply& overall_result,
//                                         const nfs::Message& message,
//                                         routing::ReplyFunctor client_reply_functor) {
//  if (overall_result.IsSuccess()) {
//    protobuf::Cost proto_cost;
//    if (!proto_cost.ParseFromString(overall_result.data().string())) {
//      return SendReplyAndAddToAccumulator(message, client_reply_functor,
//                                          nfs::Reply(CommonErrors::parsing_error));
//    }
//    nfs::Reply reply(CommonErrors::success);
//    if (low_space)
//      reply = nfs::Reply(VaultErrors::low_space);
//    AddLocalUnresolvedActionThenSync<Data, nfs::MessageAction::kPut>(
//        message,
//        proto_cost.cost());
//    SendReplyAndAddToAccumulator(message, client_reply_functor, reply);
//  } else {
//    SendReplyAndAddToAccumulator(message, client_reply_functor, overall_result);
//  }
//}

// template<typename Data>
// void MaidManagerService::CreateAccount(const MaidName& account_name, AllowedAccountCreationType)
// {
//  sync_create_accounts_.AddLocalAction();
//  DoSync();
//}

// template<typename Data, nfs::MessageAction action>
// void MaidManagerService::AddLocalUnresolvedActionThenSync(const nfs::Message& message,
//                                                          int32_t cost) {
//  auto account_name(detail::GetMaidAccountName(message));
//  auto unresolved_action(detail::CreateUnresolvedAction<Data, action>(message, cost,
//                                                                      routing_.kNodeId()));
//  maid_account_handler_.AddLocalUnresolvedAction(account_name, unresolved_action);
//  DoSync(account_name);
//}

// template<typename Data>
// void MaidManagerService::HandleVersionMessage(const nfs::Message& message,
//                                              const routing::ReplyFunctor& reply_functor) {
//}

// template<typename PublicFobType>
// void MaidManagerService::ValidatePmidRegistration(
//    const nfs::Reply& reply,
//    typename PublicFobType::Name public_fob_name,
//    std::shared_ptr<PmidRegistrationOp> pmid_registration_op) {
//  std::unique_ptr<PublicFobType> public_fob;
//  try {
//    public_fob.reset(new PublicFobType(public_fob_name,
//                                       typename PublicFobType::serialised_type(reply.data())));
//  }
//  catch(const std::exception& e) {
//    public_fob.reset();
//    LOG(kError) << e.what();
//  }
//  bool finalise(false);
//  {
//    std::lock_guard<std::mutex> lock(pmid_registration_op->mutex);
//    pmid_registration_op->SetPublicFob(std::move(public_fob));
//    finalise = (++pmid_registration_op->count == 2);
//  }
//  if (finalise)
//    FinalisePmidRegistration(pmid_registration_op);
//}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_SERVICE_H_
