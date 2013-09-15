/*  Copyright 2012 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.novinet.com/license

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#ifndef MAIDSAFE_VAULT_PMID_NODE_SERVICE_INL_H_
#define MAIDSAFE_VAULT_PMID_NODE_SERVICE_INL_H_

#include <exception>
#include <string>

#include "maidsafe/common/log.h"

#include "maidsafe/nfs/client/messages.pb.h"
#include "maidsafe/nfs/vault/messages.h"
#include "maidsafe/nfs/client/messages.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/routing/message.h"
#include "maidsafe/vault/messages.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.pb.h"
#include "maidsafe/vault/utils.h"


namespace maidsafe {

namespace vault {

template<>
void PmidNodeService::HandleMessage<nfs::GetRequestFromDataManagerToPmidNode>(
    const nfs::GetRequestFromDataManagerToPmidNode& message,
    const typename nfs::GetRequestFromDataManagerToPmidNode::Sender& sender,
    const typename nfs::GetRequestFromDataManagerToPmidNode::Receiver& receiver) {
  typedef nfs::GetRequestFromDataManagerToPmidNode MessageType;
  OperationHandlerWrapper<PmidNodeService, MessageType, nfs::PmidNodeServiceMessages>(
      accumulator_,
      [this](const MessageType& message, const typename MessageType::Sender& sender) {
        return this->ValidateSender(message, sender);
      },
      Accumulator<nfs::PmidNodeServiceMessages>::AddRequestChecker(
          RequiredRequests<MessageType>()()),
      this,
      accumulator_mutex_)(message, sender, receiver);
}

template<>
void PmidNodeService::HandleMessage<nfs::DeleteRequestFromPmidManagerToPmidNode>(
    const nfs::DeleteRequestFromPmidManagerToPmidNode& message,
    const typename nfs::DeleteRequestFromPmidManagerToPmidNode::Sender& sender,
    const typename nfs::DeleteRequestFromPmidManagerToPmidNode::Receiver& receiver) {
  typedef nfs::DeleteRequestFromPmidManagerToPmidNode MessageType;
  OperationHandlerWrapper<PmidNodeService, MessageType, nfs::PmidNodeServiceMessages>(
      accumulator_,
      [this](const MessageType& message, const typename MessageType::Sender& sender) {
        return this->ValidateSender(message, sender);
      },
      Accumulator<nfs::PmidNodeServiceMessages>::AddRequestChecker(
          RequiredRequests<MessageType>()()),
      this,
      accumulator_mutex_)(message, sender, receiver);
}

template<>
void PmidNodeService::HandleMessage(
    const nfs::GetPmidAccountResponseFromPmidManagerToPmidNode& message,
    const typename nfs::GetPmidAccountResponseFromPmidManagerToPmidNode::Sender& sender,
    const typename nfs::GetPmidAccountResponseFromPmidManagerToPmidNode::Receiver& /*receiver*/) {
#ifndef TESTNG
  ValidateSender(message, sender);
#endif
  Accumulator<nfs::PmidNodeServiceMessages>::AddResult result;
  std::vector<nfs::PmidNodeServiceMessages> responses;
  {
    std::lock_guard<std::mutex> lock(accumulator_mutex_);
    if (accumulator_.CheckHandled(message))
      return;
    auto add_request_predicate(
        [&](const std::vector<PmidNodeServiceMessages>& requests_in) {
          if (requests_in.size() < 2)
            return Accumulator<PmidNodeServiceMessages>::AddResult::kWaiting;
          std::vector<protobuf::PmidAccountResponse> pmid_account_responses;
          protobuf::PmidAccountResponse pmid_account_response;
          nfs_client::protobuf::DataNameAndContentOrReturnCode data;
          nfs::GetPmidAccountResponseFromPmidManagerToPmidNode response;
          for (auto& request : requests_in) {
            response = boost::get<nfs::GetPmidAccountResponseFromPmidManagerToPmidNode>(request);
            if (data.ParseFromString(response.contents->data->content.string())) {
              if (data.has_serialised_data_name_and_content()) {
                if (pmid_account_response.ParseFromString(
                        data.serialised_data_name_and_content())) {
                  pmid_account_responses.push_back(pmid_account_response);
                } else {
                  LOG(kWarning) << "Failed to parse the contents";
                }
              }
            } else {
              LOG(kWarning) << "Failed to parse the contents of the response";
            }
          }
          if ((static_cast<uint16_t>(requests_in.size()) >= (routing::Parameters::node_group_size / 2 + 1)) &&
               pmid_account_responses.size() >= routing::Parameters::node_group_size / 2)
            return Accumulator<PmidNodeServiceMessages>::AddResult::kSuccess;
          if ((requests_in.size() == routing::Parameters::node_group_size) ||
               (requests_in.size() - pmid_account_responses.size() >
                    routing::Parameters::node_group_size / 2))
            return Accumulator<PmidNodeServiceMessages>::AddResult::kFailure;
          return Accumulator<PmidNodeServiceMessages>::AddResult::kWaiting;
        });
    result = accumulator_.AddPendingRequest(message, sender, add_request_predicate);
    if (result == Accumulator<nfs::PmidNodeServiceMessages>::AddResult::kFailure) {
      accumulator_.SetHandled(message, sender);
      return;
    }
    if (result == Accumulator<nfs::PmidNodeServiceMessages>::AddResult::kSuccess) {
      responses = accumulator_.Get(message);
      accumulator_.SetHandled(message, sender);
    }
  }
  if (result == Accumulator<nfs::PmidNodeServiceMessages>::AddResult::kSuccess) {
    std::vector<nfs::GetPmidAccountResponseFromPmidManagerToPmidNode> typed_responses;
    for (auto response : responses)
      typed_responses.push_back(
          boost::get<nfs::GetPmidAccountResponseFromPmidManagerToPmidNode>(response));
    HandleAccountResponses(typed_responses);
  }
  if (result == Accumulator<nfs::PmidNodeServiceMessages>::AddResult::kFailure) {
    SendAccountRequest();
  }
}

// Commented by Mahmoud on 15 Sep. Needs refactoring
//template<>
//void PmidNodeService::HandleGetMessage(const nfs::GetRequestFromDataManagerToPmidNode& message,
//    const typename nfs::GetRequestFromDataManagerToPmidNode::Sender& sender,
//    const typename nfs::GetRequestFromDataManagerToPmidNode::Receiver& /*receiver*/) {
//  typedef nfs::GetResponseFromPmidNodeToDataManager NfsMessage;
//  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
//  nfs_vault::DataName data_name(message.contents->type, message.contents->raw_name);
//  try {
//    auto content(permanent_data_store_.Get(data_name));
//    NfsMessage nfs_message(nfs_client::DataNameAndContentOrReturnCode(
//        nfs_vault::DataNameAndContent(DataTagValue(message.contents->type),
//                                      message.contents->raw_name,
//                                      content)));
//    RoutingMessage routing_message(nfs_message.Serialise(),
//                                   NfsMessage::Sender(routing::SingleId(routing_.kNodeId())),
//                                   NfsMessage::Receiver(
//                                       NodeId(message.contents->raw_name.string())));
//    routing_.Send(routing_message);
//    {
//      std::lock_guard<std::mutex> lock(accumulator_mutex_);
//      accumulator_.SetHandled(message, sender);
//    }
//  } catch (const maidsafe_error& error) {
//    NfsMessage nfs_message(
//        nfs_client::DataNameAndContentOrReturnCode(
//            nfs_client::DataNameAndReturnCode(data_name, nfs_client::ReturnCode(error))));
//    RoutingMessage routing_message(nfs_message.Serialise(),
//                                   NfsMessage::Sender(routing::SingleId(routing_.kNodeId())),
//                                   NfsMessage::Receiver(
//                                       NodeId(message.contents->raw_name.string())));
//    routing_.Send(routing_message);
//    {
//      std::lock_guard<std::mutex> lock(accumulator_mutex_);
//      accumulator_.SetHandled(message, sender);
//    }
//  } catch(const std::exception& /*ex*/) {
//  }
//}

template<typename Data>
void PmidNodeService::HandleDelete(const typename Data::Name& name,
                                   const nfs::MessageId& /*message_id*/) {
  try {
    {
      handler_.permanent_data_store_.Delete(nfs_vault::DataName(name.type, name.raw_name));
      // accumulator_.SetHandled(message, sender); To be moved to OperationWrapper
    }
  } catch(const std::exception& /*ex*/) {
    std::lock_guard<std::mutex> lock(accumulator_mutex_);
    // accumulator_.SetHandled(message, sender); To be moved to OperationWrapper
  }
}


template<>
bool PmidNodeService::GetFromCache<nfs::GetRequestFromMaidNodeToDataManager>(
    const nfs::GetRequestFromMaidNodeToDataManager& message,
    const typename nfs::GetRequestFromMaidNodeToDataManager::Sender& sender,
    const typename nfs::GetRequestFromMaidNodeToDataManager::Receiver& receiver) {
  return DoGetFromCache(message, sender, receiver);
}

template<>
bool PmidNodeService::GetFromCache<nfs::GetRequestFromPmidNodeToDataManager>(
    const nfs::GetRequestFromPmidNodeToDataManager& message,
    const typename nfs::GetRequestFromPmidNodeToDataManager::Sender& sender,
    const typename nfs::GetRequestFromPmidNodeToDataManager::Receiver& receiver) {
  return DoGetFromCache(message, sender, receiver);
}

template<typename T>
bool PmidNodeService::DoGetFromCache(const T& message,
                                     const typename T::Sender& sender,
                                     const typename T::Receiver& receiver) {
  auto data_name(GetDataNameVariant(message.contents->type, message.contents->raw_name));
  if (!boost::apply_visitor(CacheableVisitor(), data_name))
    return false;
  if (boost::apply_visitor(LongTermCacheableVisitor(), data_name))
    return CacheGet(message, sender, receiver, IsLongTermCacheable());
  return CacheGet(message, sender, receiver, IsShortTermCacheable());
}

template<typename T>
bool PmidNodeService::CacheGet(const T& message,
                               const typename T::Sender& sender,
                               const typename T::Receiver& receiver,
                               IsShortTermCacheable) {
  try {
    auto content(std::make_shared<NonEmptyString>(handler_.mem_only_cache_.Get(*message.contents)));
    active_.Send([=]() {
                   SendCachedData(message, sender, receiver, content);
                 });
  } catch(maidsafe_error& /*error*/) {
    return false;
  }
  return true;
}

template<typename T>
bool PmidNodeService::CacheGet(const T& message,
                               const typename T::Sender& sender,
                               const typename T::Receiver& receiver,
                               IsLongTermCacheable) {
  try {
    auto content(std::make_shared<NonEmptyString>(handler_.cache_data_store_.Get(*message.contents)));
    active_.Send([=]() {
                   SendCachedData(message, sender, receiver, content);
                 });
  } catch(maidsafe_error& /*error*/) {
    return false;
  }
  return true;
}

template<>
void PmidNodeService::StoreInCache<nfs::GetResponseFromDataManagerToMaidNode>(
    const nfs::GetResponseFromDataManagerToMaidNode& message,
    const typename nfs::GetResponseFromDataManagerToMaidNode::Sender& /*sender*/,
    const typename nfs::GetResponseFromDataManagerToMaidNode::Receiver& /*receiver*/) {
  auto data_name(GetDataNameVariant(message.contents->data->name.type,
                                    message.contents->data->name.raw_name));
  if (!boost::apply_visitor(CacheableVisitor(), data_name))
    return;
  if (boost::apply_visitor(LongTermCacheableVisitor(), data_name))
    CacheStore(message, data_name, IsLongTermCacheable());
  else
    CacheStore(message, data_name, IsShortTermCacheable());
}

template<typename T>
void PmidNodeService::CacheStore(const T& message,
                                 const DataNameVariant& data_name,
                                 IsShortTermCacheable) {
  handler_.mem_only_cache_.Store(data_name, message.contents->data->content);
}

template<typename T>
void PmidNodeService::CacheStore(const T& message,
                                 const DataNameVariant& name,
                                 IsLongTermCacheable) {
  handler_.cache_data_store_.Store(name, message.contents->data->content);
}

template <typename T>
void PmidNodeService::SendCachedData(const T& message,
                                     const typename T::Sender& sender,
                                     const typename T::Receiver& /*receiver*/,
                                     const std::shared_ptr<NonEmptyString> content) {
  typedef nfs::GetCachedResponseFromPmidNodeToMaidNode NfsMessage;
  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
  NfsMessage nfs_message(nfs_client::DataNameAndContentOrReturnCode(
      nfs_vault::DataNameAndContent(DataTagValue(message.contents->type),
                                    message.contents->raw_name,
                                    *content)));
  RoutingMessage routing_message(nfs_message.Serialise(),
                                 NfsMessage::Sender(routing::SingleId(routing_.kNodeId())),
                                 NfsMessage::Receiver(sender));
  routing_.Send(routing_message);
}

// Commented by Mahmoud on 15 Sep. MUST BE FIXED
//template<>
//bool PmidNodeService::ValidateSender(
//    const nfs::PutRequestFromPmidManagerToPmidNode& message,
//    const typename nfs::PutRequestFromPmidManagerToPmidNode::Sender& /*sender*/) const {
//  if (!SenderIsConnectedVault(message, routing_))
//    ThrowError(VaultErrors::permission_denied);

//  if (!FromPmidManager(message) || !ForThisPersona(message))
//    ThrowError(CommonErrors::invalid_parameter);
//}

//template<>
//bool PmidNodeService::ValidateSender(
//    const nfs::GetRequestFromDataManagerToPmidNode& message,
//    const typename nfs::GetRequestFromDataManagerToPmidNode::Sender& /*sender*/) const {
//  if (!SenderInGroupForMetadata(message, routing_))
//    ThrowError(VaultErrors::permission_denied);

//  if (!FromDataManager(message) || !ForThisPersona(message))
//    ThrowError(CommonErrors::invalid_parameter);
//}

//template<>
//bool PmidNodeService::ValidateSender(
//    const nfs::DeleteRequestFromPmidManagerToPmidNode& message,
//    const typename nfs::DeleteRequestFromPmidManagerToPmidNode::Sender& /*sender*/) const {
//  if (!SenderIsConnectedVault(message, routing_))
//    ThrowError(VaultErrors::permission_denied);

//  if (!FromPmidManager(message) || !ForThisPersona(message))
//    ThrowError(CommonErrors::invalid_parameter);
//}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_NODE_SERVICE_INL_H_
