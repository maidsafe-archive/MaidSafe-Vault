/* Copyright 2012 MaidSafe.net limited

This MaidSafe Software is licensed under the MaidSafe.net Commercial License, version 1.0 or later,
and The General Public License (GPL), version 3. By contributing code to this project You agree to
the terms laid out in the MaidSafe Contributor Agreement, version 1.0, found in the root directory
of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available at:

http://www.novinet.com/license

Unless required by applicable law or agreed to in writing, software distributed under the License is
distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.
*/

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_MAID_MANAGER_SERVICE_INL_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_MAID_MANAGER_SERVICE_INL_H_

#include <exception>
#include <string>

#include "maidsafe/common/error.h"
#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/common/on_scope_exit.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/nfs/utils.h"
#include "maidsafe/nfs/reply.h"

#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/maid_manager/maid_account.pb.h"


namespace maidsafe {

namespace vault {

namespace detail {

template<typename T>
struct can_create_account : public std::false_type {};

template<>
struct can_create_account<passport::PublicAnmaid> : public std::true_type {};

template<>
struct can_create_account<passport::PublicMaid> : public std::true_type {};

template<typename Data>
int32_t EstimateCost(const Data& data) {
  static_assert(!std::is_same<Data, passport::PublicAnmaid>::value, "Cost of Anmaid should be 0.");
  static_assert(!std::is_same<Data, passport::PublicMaid>::value, "Cost of Maid should be 0.");
  static_assert(!std::is_same<Data, passport::PublicPmid>::value, "Cost of Pmid should be 0.");
  return static_cast<int32_t>(MaidAccountHolderService::DefaultPaymentFactor() *
                              data.content.string().size());
}

template<>
int32_t EstimateCost<passport::PublicAnmaid>(const passport::PublicAnmaid&);

template<>
int32_t EstimateCost<passport::PublicMaid>(const passport::PublicMaid&);

template<>
int32_t EstimateCost<passport::PublicPmid>(const passport::PublicPmid&);

MaidName GetMaidAccountName(const nfs::Message& message);

template<typename Data>
typename Data::name_type GetDataName(const nfs::Message& message) {
  // Hash the data name to obfuscate the list of chunks associated with the client.
  return typename Data::name_type(crypto::Hash<crypto::SHA512>(message.data().name));
}

template<typename Data, nfs::MessageAction action>
MaidAccountUnresolvedEntry CreateUnresolvedEntry(const nfs::Message& message,
                                                 int32_t cost,
                                                 const NodeId& this_id) {
  static_assert(action == nfs::MessageAction::kPut || action == nfs::MessageAction::kDelete,
                "Action must be either kPut of kDelete.");
  return MaidAccountUnresolvedEntry(
      std::make_pair(DataNameVariant(GetDataName<Data>(message)), action), cost, this_id);
}

}  // namespace detail


template<typename Data>
void MaidAccountHolderService::HandleMessage(const nfs::Message& message,
                                             const routing::ReplyFunctor& reply_functor) {
  ValidateDataSender(message);
  nfs::Reply reply(CommonErrors::success);
  {
    std::lock_guard<std::mutex> lock(accumulator_mutex_);
    if (accumulator_.CheckHandled(message, reply))
      return reply_functor(reply.Serialise()->string());
  }
  if (message.data().action == nfs::MessageAction::kPut) {
    HandlePut<Data>(message, reply_functor);
  } else if (message.data().action == nfs::MessageAction::kDelete) {
    HandleDelete<Data>(message, reply_functor);
  } else {
    reply = nfs::Reply(VaultErrors::operation_not_supported, message.Serialise().data);
    SendReplyAndAddToAccumulator(message, reply_functor, reply);
  }
}

template<typename Data>
void MaidAccountHolderService::HandlePut(const nfs::Message& message,
                                         const routing::ReplyFunctor& reply_functor) {
  maidsafe_error return_code(CommonErrors::success);
  try {
    Data data(typename Data::name_type(message.data().name),
              typename Data::serialised_type(message.data().content));
    auto account_name(detail::GetMaidAccountName(message));
    auto estimated_cost(detail::EstimateCost(message.data()));
    maid_account_handler_.CreateAccount<Data>(account_name, detail::can_create_account<Data>());
    auto account_status(maid_account_handler_.AllowPut(account_name, estimated_cost));

    if (account_status == MaidAccount::Status::kNoSpace)
      ThrowError(VaultErrors::not_enough_space);
    bool low_space(account_status == MaidAccount::Status::kLowSpace);

    auto put_op(std::make_shared<nfs::OperationOp>(
        kPutRepliesSuccessesRequired_,
        [this, message, reply_functor, low_space](nfs::Reply overall_result) {
            this->HandlePutResult<Data>(overall_result, message, reply_functor, low_space,
                                        is_unique_on_network<Data>());
        }));

    if (low_space)
      UpdatePmidTotals(account_name);

    nfs_.Put(data,
             message.pmid_node(),
             [put_op](std::string serialised_reply) {
                 nfs::HandleOperationReply(put_op, serialised_reply);
             });
    return SendEarlySuccessReply<Data>(message, reply_functor, low_space,
                                       is_unique_on_network<Data>());
  }
  catch(const maidsafe_error& error) {
    LOG(kWarning) << error.what();
    return_code = error;
  }
  catch(...) {
    LOG(kWarning) << "Unknown error.";
    return_code = MakeError(CommonErrors::unknown);
  }
  nfs::Reply reply(return_code, message.Serialise().data);
  SendReplyAndAddToAccumulator(message, reply_functor, reply);
}

template<typename Data>
void MaidAccountHolderService::HandleDelete(const nfs::Message& message,
                                            const routing::ReplyFunctor& reply_functor) {
  SendReplyAndAddToAccumulator(message, reply_functor, nfs::Reply(CommonErrors::success));
  try {
    auto account_name(detail::GetMaidAccountName(message));
    typename Data::name_type data_name(message.data().name);
    maid_account_handler_.DeleteData<Data>(account_name, data_name);
    AddLocalUnresolvedEntryThenSync<Data, nfs::MessageAction::kDelete>(message, 0);
    nfs_.Delete<Data>(data_name, [](std::string /*serialised_reply*/) {});
  }
  catch(const maidsafe_error& error) {
    LOG(kWarning) << error.what();
    // Always return success for Deletes
  }
  catch(...) {
    LOG(kWarning) << "Unknown error.";
    // Always return success for Deletes
  }
}

template<typename Data>
void MaidAccountHolderService::SendEarlySuccessReply(const nfs::Message& message,
                                                     const routing::ReplyFunctor& reply_functor,
                                                     bool low_space,
                                                     NonUniqueDataType) {
  nfs::Reply reply(CommonErrors::success);
  if (low_space)
    reply = nfs::Reply(VaultErrors::low_space);
  SendReplyAndAddToAccumulator(message, reply_functor, reply);
}

template<typename Data>
void MaidAccountHolderService::HandlePutResult(const nfs::Reply& overall_result,
                                               const nfs::Message& message,
                                               routing::ReplyFunctor client_reply_functor,
                                               bool low_space,
                                               UniqueDataType) {
  if (overall_result.IsSuccess()) {
    nfs::Reply reply(CommonErrors::success);
    if (low_space)
      reply = nfs::Reply(VaultErrors::low_space);
    AddLocalUnresolvedEntryThenSync<Data, nfs::MessageAction::kPut>(
        message,
        detail::EstimateCost(message.data()));
    SendReplyAndAddToAccumulator(message, client_reply_functor, reply);
  } else {
    SendReplyAndAddToAccumulator(message, client_reply_functor, overall_result);
  }
}

template<typename Data>
void MaidAccountHolderService::HandlePutResult(const nfs::Reply& overall_result,
                                               const nfs::Message& message,
                                               routing::ReplyFunctor /*client_reply_functor*/,
                                               bool /*low_space*/,
                                               NonUniqueDataType) {
  try {
    if (overall_result.IsSuccess()) {
      protobuf::Cost proto_cost;
      proto_cost.ParseFromString(overall_result.data().string());
      // TODO(Fraser#5#): 2013-05-09 - The client's reply should only be sent *after* this call.
      AddLocalUnresolvedEntryThenSync<Data, nfs::MessageAction::kPut>(message, proto_cost.cost());
    }
  }
  catch(const std::exception& e) {
    LOG(kError) << "Failed to Handle Put result: " << e.what();
  }
}

template<typename Data, nfs::MessageAction action>
void MaidAccountHolderService::AddLocalUnresolvedEntryThenSync(const nfs::Message& message,
                                                               int32_t cost) {
  auto account_name(detail::GetMaidAccountName(message));
  auto unresolved_entry(detail::CreateUnresolvedEntry<Data, action>(message, cost,
                                                                    routing_.kNodeId()));
  maid_account_handler_.AddLocalUnresolvedEntry(account_name, unresolved_entry);
  Sync(account_name);
}

template<typename PublicFobType>
void MaidAccountHolderService::ValidatePmidRegistration(
    const nfs::Reply& reply,
    typename PublicFobType::name_type public_fob_name,
    std::shared_ptr<PmidRegistrationOp> pmid_registration_op) {
  std::unique_ptr<PublicFobType> public_fob;
  try {
    public_fob.reset(new PublicFobType(public_fob_name,
                                       typename PublicFobType::serialised_type(reply.data())));
  }
  catch(const std::exception& e) {
    public_fob.reset();
    LOG(kError) << e.what();
  }
  bool finalise(false);
  {
    std::lock_guard<std::mutex> lock(pmid_registration_op->mutex);
    pmid_registration_op->SetPublicFob(std::move(public_fob));
    finalise = (++pmid_registration_op->count == 2);
  }
  if (finalise)
    FinalisePmidRegistration(pmid_registration_op);
}


}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_MAID_MANAGER_SERVICE_INL_H_
