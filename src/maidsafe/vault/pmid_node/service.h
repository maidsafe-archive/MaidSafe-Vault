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

#ifndef MAIDSAFE_VAULT_PMID_NODE_SERVICE_H_
#define MAIDSAFE_VAULT_PMID_NODE_SERVICE_H_

#include <mutex>
#include <type_traits>
#include <set>
#include <vector>
#include <string>
#include <functional>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "maidsafe/common/types.h"
#include "maidsafe/common/active.h"
#include "maidsafe/data_types/data_type_values.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/message_types.h"
#include "maidsafe/nfs/client/data_getter.h"

#include "maidsafe/vault/message_types.h"
#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/integrity_check_data.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.pb.h"
#include "maidsafe/vault/pmid_node/handler.h"
#include "maidsafe/vault/pmid_node/dispatcher.h"
#include "maidsafe/vault/operation_visitors.h"

namespace maidsafe {

namespace vault {

namespace protobuf {
class PmidAccountResponse;
}

namespace test {

class PmidNodeServiceTest;

}  // namespace test

namespace {

template <typename T>
class HasData {
  typedef char Yes;
  typedef long No;

  template <typename C>
  static Yes Check(decltype(&C::data));
  template <typename C>
  static No Check(...);

 public:
  static bool const value = sizeof(Check<T>(0)) == sizeof(Yes);
};

template <bool>
struct message_has_data : public std::false_type {};

template <>
struct message_has_data<true> : public std::true_type {};

class ContentRetrievalVisitor : public boost::static_visitor<std::string> {
 public:
  template <typename T>
  result_type operator()(const T& message) {
    return GetData(message, message_has_data<HasData<T>::value>());
  }

 private:
  template <typename T>
  result_type GetData(const T& message, std::true_type) {
    return message.data();
  }

  template <typename T>
  result_type GetData(const T& /*message*/, std::false_type) {
    return result_type();
  }
};

class GetCallerVisitor : public boost::static_visitor<> {
 public:
//  typedef std::function<void(const DataNameVariant& key, const NonEmptyString& value)>
//      DataStoreFunctor;
//  GetCallerVisitor(nfs_client::DataGetter& data_getter, std::vector<std::future<void>>& futures,
//                   DataStoreFunctor store_functor)
//      : data_getter_(data_getter), futures_(futures), store_functor_(store_functor) {}

//  template <typename DataName>
//  void operator()(const DataName& data_name) {
//    auto future(std::async(std::launch::async, [&] {
//      data_getter_.Get<typename DataName::data_type>(data_name, std::chrono::seconds(10));
//    }));
//    futures_.push_back(
//        future.then([this, data_name](std::future<typename DataName::data_type> result_future) {
//          auto result(result_future.get());
//          auto content(boost::apply_visitor(ContentRetrievalVisitor(), result));
//          try {
//            if (!content.empty())
//              store_functor_(data_name, NonEmptyString(content));
//          }
//          catch (const maidsafe_error& /*error*/) {
//          }
//        }));
//  }

// private:
//  nfs_client::DataGetter& data_getter_;
//  std::vector<std::future<void>>& futures_;
//  DataStoreFunctor store_functor_;
};

}  // noname namespace

class PmidNodeService {
 public:
  typedef void PublicMessages;
  typedef PmidNodeServiceMessages VaultMessages;
  typedef PmidNodeServiceMessages Messages;
  typedef void HandleMessageReturnType;

  PmidNodeService(const passport::Pmid& pmid, routing::Routing& routing,
                  nfs_client::DataGetter& data_getter,
                  const boost::filesystem::path& vault_root_dir);

  template <typename MessageType>
  void HandleMessage(const MessageType& message, const typename MessageType::Sender& sender,
                     const typename MessageType::Receiver& receiver);

  void HandleChurnEvent(std::shared_ptr<routing::MatrixChange> /*matrix_change*/) {}  // No-op

  template <typename Data>
  void HandleDelete(const typename Data::Name& data_name);

  // Unless StartUp is called, PmidNode is not un-usable
  void StartUp();
  void HandlePmidAccountResponses(const std::vector<std::set<nfs_vault::DataName>>& responses,
                                  int failures);

 private:
  friend class detail::PmidNodeDeleteVisitor<PmidNodeService>;
  friend class detail::PmidNodePutVisitor<PmidNodeService>;
  friend class detail::PmidNodeGetVisitor<PmidNodeService>;
  friend class detail::PmidNodeIntegrityCheckVisitor<PmidNodeService>;
  friend class test::PmidNodeServiceTest;

  // ================================ Pmid Account ===============================================

  // populates chunks map
  //  void UpdateLocalStorage(const std::map<DataNameVariant, uint16_t>& expected_files);
  void UpdateLocalStorage(const std::vector<DataNameVariant>& to_be_deleted,
                          const std::vector<DataNameVariant>& to_be_retrieved);
  void CheckPmidAccountResponsesStatus(const std::vector<DataNameVariant>& expected_chunks);

  std::future<std::unique_ptr<ImmutableData>> RetrieveFileFromNetwork(
      const DataNameVariant& file_id);
  void HandleAccountResponses(
      const std::vector<GetPmidAccountResponseFromPmidManagerToPmidNode>& responses);
  template <typename Data>
  void HandlePut(const Data& data, nfs::MessageId message_id);
  template <typename Data>
  void HandleGet(const typename Data::Name& data_name, const NodeId& data_manager_node_id,
                 nfs::MessageId message_id);
  template <typename Data>
  void HandleIntegrityCheck(const typename Data::Name& data_name,
                            const NonEmptyString& random_string, const NodeId& sender,
                            nfs::MessageId message_id);

  // ================================ Sender Validation =========================================
  template <typename T>
  bool ValidateSender(const T& /*message*/, const typename T::Sender& /*sender*/) const {
    return true;
  }

  // ===================================Cache=====================================================
  template <typename T>
  bool DoGetFromCache(const T& message, const typename T::Sender& sender,
                      const typename T::Receiver& receiver);

  template <typename T>
  void SendCachedData(const T& message, const typename T::Sender& sender,
                      const typename T::Receiver& receiver,
                      const std::shared_ptr<NonEmptyString> content);

  routing::Routing& routing_;
  std::mutex accumulator_mutex_;
#ifdef USE_MAL_BEHAVIOUR
  uint32_t malfunc_behaviour_seed_;
#endif
  Accumulator<Messages> accumulator_;
  PmidNodeDispatcher dispatcher_;
  PmidNodeHandler handler_;
  Active active_;
  nfs_client::DataGetter& data_getter_;
};

template <typename MessageType>
void PmidNodeService::HandleMessage(const MessageType& /*message*/,
                                    const typename MessageType::Sender& /*sender*/,
                                    const typename MessageType::Receiver& /*receiver*/) {
// BEFORE_RELEASE uncomment below line
//  MessageType::invalid_message_type_passed___should_be_one_of_the_specialisations_defined_below;
}

template <>
void PmidNodeService::HandleMessage(
    const PutRequestFromPmidManagerToPmidNode& message,
    const typename PutRequestFromPmidManagerToPmidNode::Sender& sender,
    const typename PutRequestFromPmidManagerToPmidNode::Receiver& receiver);

template<>
void PmidNodeService::HandleMessage(
    const GetRequestFromDataManagerToPmidNode& message,
    const typename GetRequestFromDataManagerToPmidNode::Sender& sender,
    const typename GetRequestFromDataManagerToPmidNode::Receiver& receiver);

template <>
void PmidNodeService::HandleMessage(
    const IntegrityCheckRequestFromDataManagerToPmidNode& message,
    const typename IntegrityCheckRequestFromDataManagerToPmidNode::Sender& sender,
    const typename IntegrityCheckRequestFromDataManagerToPmidNode::Receiver& receiver);

template<>
void PmidNodeService::HandleMessage(
    const DeleteRequestFromPmidManagerToPmidNode& message,
    const typename DeleteRequestFromPmidManagerToPmidNode::Sender& sender,
    const typename DeleteRequestFromPmidManagerToPmidNode::Receiver& receiver);

template <>
void PmidNodeService::HandleMessage(
    const GetPmidAccountResponseFromPmidManagerToPmidNode& message,
    const typename GetPmidAccountResponseFromPmidManagerToPmidNode::Sender& sender,
    const typename GetPmidAccountResponseFromPmidManagerToPmidNode::Receiver& receiver);

template <>
void PmidNodeService::HandleMessage(
    const PmidHealthRequestFromPmidManagerToPmidNode& message,
    const typename PmidHealthRequestFromPmidManagerToPmidNode::Sender& sender,
    const typename PmidHealthRequestFromPmidManagerToPmidNode::Receiver& receiver);

// ============================== Get implementation =============================================
template <typename Data>
void PmidNodeService::HandleGet(const typename Data::Name& data_name,
                                const NodeId& data_manager_node_id,
                                nfs::MessageId message_id) {
  try {
    auto data(handler_.Get<Data>(data_name));
#ifdef USE_MAL_BEHAVIOUR
    LOG(kVerbose) << "PmidNodeService::HandleGet malfunc_behaviour_seed_ is "
                  << malfunc_behaviour_seed_;
    if ((malfunc_behaviour_seed_ % 4) == 0) {
      LOG(kVerbose) << "PmidNodeService::HandleGet generating an incorrect get response";
      IntegrityCheckData integrity_check_data(RandomString(64), data.Serialise());
      nfs_vault::DataNameAndContentOrCheckResult data_or_check_result(
          Data::Name::data_type::Tag::kValue, data.name().value, integrity_check_data.result());
      dispatcher_.SendGetOrIntegrityCheckResponse(data_or_check_result, data_manager_node_id,
                                                  message_id);
      return;
    }
#else
    nfs_vault::DataNameAndContentOrCheckResult
        data_or_check_result(Data::Name::data_type::Tag::kValue,
                             data.name().value, data.Serialise());
    LOG(kVerbose) << "PmidNodeService::HandleGet got " << HexSubstr(data.name().value) 
                  << " with content " << HexSubstr(data.Serialise().data);
    dispatcher_.SendGetOrIntegrityCheckResponse(data_or_check_result, data_manager_node_id,
                                                message_id);
#endif
  } catch (const maidsafe_error& error) {
    // Not sending error here as timeout will happen anyway at Datamanager.
    // This case should be least frequent.
    LOG(kError) << "Failed to get data : " << DebugId(data_name.value) << " , "
                << error.what();
  } catch (const std::exception& e) {
    LOG(kError) << "Failed to get data : " << DebugId(data_name.value) << " , "
                << e.what();
  }
}

// ============================== Put implementation =============================================
template <typename Data>
void PmidNodeService::HandlePut(const Data& data, nfs::MessageId message_id) {
  try {
    LOG(kVerbose) << "PmidNodeService::HandlePut put " << HexSubstr(data.name().value)
                  << " with message_id " << message_id.data;
    handler_.Put(data);
  }
  catch (const maidsafe_error& error) {
    LOG(kVerbose) << "PmidNodeService::HandlePut send put failure " << HexSubstr(data.name().value)
                  << " with AvailableSpace " << handler_.AvailableSpace()
                  << " and error " << error.what();
    dispatcher_.SendPutFailure<Data>(data.name(), handler_.AvailableSpace(), error, message_id);
  }
}

template <typename Data>
void PmidNodeService::HandleDelete(const typename Data::Name& data_name) {
  try {
    LOG(kVerbose) << "PmidNodeService::HandleDelete delete " << HexSubstr(data_name.value);
    handler_.Delete(GetDataNameVariant(Data::Tag::kValue, data_name.value));
  }
  catch (const maidsafe_error& /*error*/) {
  }
}

template <typename Data>
void PmidNodeService::HandleIntegrityCheck(const typename Data::Name& data_name,
                                           const NonEmptyString& random_string,
                                           const NodeId& data_manager_node_id,
                                           nfs::MessageId message_id) {
  try {
    auto data(handler_.Get<Data>(data_name));
    std::string random_seed(random_string.string());
#ifdef USE_MAL_BEHAVIOUR
    LOG(kVerbose) << "PmidNodeService::HandleIntegrityCheck malfunc_behaviour_seed_ is "
                  << malfunc_behaviour_seed_;
    if ((malfunc_behaviour_seed_ % 4) == 0) {
      LOG(kVerbose) << "PmidNodeService::HandleIntegrityCheck generating an incorrect response";
      random_seed = RandomString(64);
    }
#endif
    IntegrityCheckData integrity_check_data(random_seed, data.Serialise());
    nfs_vault::DataNameAndContentOrCheckResult
        data_or_check_result(Data::Name::data_type::Tag::kValue, data.name().value,
                                 integrity_check_data.result());
    LOG(kVerbose) << "PmidNodeService::HandleIntegrityCheck send back integrity_check_data for "
                  << HexSubstr(data.name().value);
    dispatcher_.SendGetOrIntegrityCheckResponse(data_or_check_result, data_manager_node_id,
                                                message_id);
  } catch (const maidsafe_error& error) {
    // Not sending error here as timeout will happen anyway at Datamanager.
    // This case should be least frequent.
    LOG(kError) << "Failed to do integrity check for data : " << DebugId(data_name.value) << " , "
                << error.what();
  }
}

//template <typename Data>
//void PmidNodeService::HandleIntegrityCheck(const typename Data::Name& data_name,
//                                           const NonEmptyString& random_string,
//                                           const NodeId& sender,
//                                           nfs::MessageId message_id) {
//  try {
//    auto content(
//        handler_.GetFromPermanentStore(GetDataNameVariant(data_name.type, data_name.raw_name)));
//    NonEmptyString signature(crypto::Hash<crypto::SHA512>(NonEmptyString(content + random_string)));
//    dispatcher_.SendIntegrityCheckResponse(data_name, signature, sender, CommonErrors::success,
//                                           message_id);
//  }
//  catch (const maidsafe_error& error) {
//    dispatcher_.SendIntegrityCheckResponse(data_name, std::string(), sender, error, message_id);
//  }
//}

// template<>
// bool PmidNodeService::GetFromCache<nfs::GetRequestFromMaidNodeToDataManager>(
//    const nfs::GetRequestFromMaidNodeToDataManager& message,
//    const typename nfs::GetRequestFromMaidNodeToDataManager::Sender& sender,
//    const typename nfs::GetRequestFromMaidNodeToDataManager::Receiver& receiver) {
//  return DoGetFromCache(message, sender, receiver);
//}

// template<typename T>
// bool PmidNodeService::DoGetFromCache(const T& message,
//                                     const typename T::Sender& sender,
//                                     const typename T::Receiver& receiver) {
//  auto data_name(GetDataNameVariant(message.contents->type, message.contents->raw_name));
//  if (!boost::apply_visitor(CacheableVisitor(), data_name))
//    return false;
//  if (boost::apply_visitor(LongTermCacheableVisitor(), data_name))
//    return CacheGet(message, sender, receiver, IsLongTermCacheable());
//  return CacheGet(message, sender, receiver, IsShortTermCacheable());
//}

// template<typename T>
// bool PmidNodeService::CacheGet(const T& message,
//                               const typename T::Sender& sender,
//                               const typename T::Receiver& receiver,
//                               IsShortTermCacheable) {
//  try {
//    auto
// content(std::make_shared<NonEmptyString>(handler_.mem_only_cache_.Get(*message.contents)));
//    active_.Send([=]() {
//                   SendCachedData(message, sender, receiver, content);
//                 });
//  } catch(maidsafe_error& /*error*/) {
//    return false;
//  }
//  return true;
//}

// template<typename T>
// bool PmidNodeService::CacheGet(const T& message,
//                               const typename T::Sender& sender,
//                               const typename T::Receiver& receiver,
//                               IsLongTermCacheable) {
//  try {
//    auto
// content(std::make_shared<NonEmptyString>(handler_.cache_data_store_.Get(*message.contents)));
//    active_.Send([=]() {
//                   SendCachedData(message, sender, receiver, content);
//                 });
//  } catch(maidsafe_error& /*error*/) {
//    return false;
//  }
//  return true;
//}

// template<>
// void PmidNodeService::StoreInCache<nfs::GetResponseFromDataManagerToMaidNode>(
//    const nfs::GetResponseFromDataManagerToMaidNode& message,
//    const typename nfs::GetResponseFromDataManagerToMaidNode::Sender& /*sender*/,
//    const typename nfs::GetResponseFromDataManagerToMaidNode::Receiver& /*receiver*/) {
//  auto data_name(GetDataNameVariant(message.contents->data->name.type,
//                                    message.contents->data->name.raw_name));
//  if (!boost::apply_visitor(CacheableVisitor(), data_name))
//    return;
//  if (boost::apply_visitor(LongTermCacheableVisitor(), data_name))
//    CacheStore(message, data_name, IsLongTermCacheable());
//  else
//    CacheStore(message, data_name, IsShortTermCacheable());
//}

// template<typename T>
// void PmidNodeService::CacheStore(const T& message,
//                                 const DataNameVariant& data_name,
//                                 IsShortTermCacheable) {
//  handler_.mem_only_cache_.Store(data_name, message.contents->data->content);
//}

// template<typename T>
// void PmidNodeService::CacheStore(const T& message,
//                                 const DataNameVariant& name,
//                                 IsLongTermCacheable) {
//  handler_.cache_data_store_.Store(name, message.contents->data->content);
//}

// template <typename T>
// void PmidNodeService::SendCachedData(const T& message,
//                                     const typename T::Sender& sender,
//                                     const typename T::Receiver& /*receiver*/,
//                                     const std::shared_ptr<NonEmptyString> content) {
//  typedef nfs::GetCachedResponseFromPmidNodeToMaidNode NfsMessage;
//  typedef routing::Message<NfsMessage::Sender, NfsMessage::Receiver> RoutingMessage;
//  NfsMessage nfs_message(nfs_client::DataNameAndContentOrReturnCode(
//      nfs_vault::DataNameAndContent(DataTagValue(message.contents->type),
//                                    message.contents->raw_name,
//                                    *content)));
//  RoutingMessage routing_message(nfs_message.Serialise(),
//                                 NfsMessage::Sender(routing::SingleId(routing_.kNodeId())),
//                                 NfsMessage::Receiver(sender));
//  routing_.Send(routing_message);
//}

// Commented by Mahmoud on 15 Sep. MUST BE FIXED
// template<>
// bool PmidNodeService::ValidateSender(
//    const PutRequestFromPmidManagerToPmidNode& message,
//    const typename PutRequestFromPmidManagerToPmidNode::Sender& /*sender*/) const {
//  if (!SenderIsConnectedVault(message, routing_))
//    ThrowError(VaultErrors::permission_denied);

//  if (!FromPmidManager(message) || !ForThisPersona(message))
//    ThrowError(CommonErrors::invalid_parameter);
//}

// template<>
// bool PmidNodeService::ValidateSender(
//    const nfs::GetRequestFromDataManagerToPmidNode& message,
//    const typename nfs::GetRequestFromDataManagerToPmidNode::Sender& /*sender*/) const {
//  if (!SenderInGroupForMetadata(message, routing_))
//    ThrowError(VaultErrors::permission_denied);

//  if (!FromDataManager(message) || !ForThisPersona(message))
//    ThrowError(CommonErrors::invalid_parameter);
//}

// template<>
// bool PmidNodeService::ValidateSender(
//    const DeleteRequestFromPmidManagerToPmidNode& message,
//    const typename DeleteRequestFromPmidManagerToPmidNode::Sender& /*sender*/) const {
//  if (!SenderIsConnectedVault(message, routing_))
//    ThrowError(VaultErrors::permission_denied);

//  if (!FromPmidManager(message) || !ForThisPersona(message))
//    ThrowError(CommonErrors::invalid_parameter);
//}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_NODE_SERVICE_H_
