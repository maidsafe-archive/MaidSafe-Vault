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
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/message_types.h"
#include "maidsafe/nfs/client/data_getter.h"
#include "maidsafe/vault/message_types.h"
#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.pb.h"
#include "maidsafe/vault/pmid_node/handler.h"
#include "maidsafe/vault/pmid_node/dispatcher.h"


namespace maidsafe {

namespace vault {

namespace protobuf { class PmidAccountResponse; }

namespace test {

template<typename Data>
class DataHolderTest;

}  // namespace test

namespace {

template <typename T>
class HasData {
  typedef char Yes;
  typedef long No;

  template <typename C> static Yes Check(decltype(&C::data)) ;
  template <typename C> static No Check(...);

 public:
    static bool const value = sizeof(Check<T>(0)) == sizeof(Yes);
};

template <bool>
struct message_has_data : public std::false_type {};

template <>
struct message_has_data<true> : public std::true_type {};

class ContentRetrievalVisitor : public boost::static_visitor<std::string> {
 public:
  template<typename T>
  result_type operator()(const T& message) {
    return GetData(message, message_has_data<HasData<T>::value>());
  }

 private:
  template<typename T>
  result_type GetData(const T& message, std::true_type) {
    return message.data();
  }

  template<typename T>
  result_type GetData(const T& /*message*/, std::false_type) {
    return result_type();
  }
};

class GetCallerVisitor : public boost::static_visitor<> {
 public:
  typedef std::function<void(const DataNameVariant& key,
                             const NonEmptyString& value)> DataStoreFunctor;
  GetCallerVisitor(nfs_client::DataGetter& data_getter,
                   std::vector<std::future<void>>& futures,
                   DataStoreFunctor store_functor)
      : data_getter_(data_getter), futures_(futures), store_functor_(store_functor) {}

  template<typename DataName>
  void operator()(const DataName& data_name) {
    auto future(std::async(std::launch::async,
                           [&] {
                             data_getter_.Get<typename DataName::data_type>(
                                 data_name,
                                 std::chrono::seconds(10));
                           }));
    futures_.push_back(future.then(
        [this, data_name](std::future<typename DataName::data_type> result_future) {
          auto result(result_future.get());
          auto content(boost::apply_visitor(ContentRetrievalVisitor(), result));
          try {
            if (!content.empty())
              store_functor_(data_name, NonEmptyString(content));
          } catch (const maidsafe_error& /*error*/) {}
        }));
  }
 private:
  nfs_client::DataGetter& data_getter_;
  std::vector<std::future<void>>& futures_;
  DataStoreFunctor store_functor_;
};

class LongTermCacheableVisitor : public boost::static_visitor<bool> {
  public:
   template<typename Data>
   void operator()() {
     return is_long_term_cacheable<Data>::value;
   }
};

class CacheableVisitor : public boost::static_visitor<bool> {
  public:
   template<typename Data>
   void operator()() {
     return is_cacheable<Data>::value;
   }
};

}  // noname namespace

class PmidNodeService {
 public:
  typedef nfs::PmidNodeServiceMessages PublicMessages;
  typedef nfs::PmidNodeServiceMessages VaultMessages; // FIXME (Check with Fraser)

  enum : uint32_t { kPutRequestsRequired = 3, kDeleteRequestsRequired = 3 };

  PmidNodeService(const passport::Pmid& pmid,
                  routing::Routing& routing,
                  const boost::filesystem::path& vault_root_dir);

  template<typename T>
  void HandleMessage(const T& message,
                     const typename T::Sender& sender,
                     const typename T::Receiver& receiver);

  template<typename T>
  bool GetFromCache(const T& /*message*/,
                    const typename T::Sender& /*sender*/,
                    const typename T::Receiver& /*receiver*/) {
    T::invalid_message_type_passed::should_be_one_of_the_specialisations_defined_below;
    return false;
  }

  template<typename T>
  void StoreInCache(const T& /*message*/,
                    const typename T::Sender& /*sender*/,
                    const typename T::Receiver& /*receiver*/) {
    T::invalid_message_type_passed::should_be_one_of_the_specialisations_defined_below;
  }

  template<typename Data>
  friend class test::DataHolderTest;

 private:
  typedef std::true_type IsCacheable, IsLongTermCacheable;
  typedef std::false_type IsNotCacheable, IsShortTermCacheable;

// ================================ Pmid Account ===============================================
  void SendAccountRequest();

  // populates chunks map
//  void ApplyAccountTransfer(const std::vector<protobuf::PmidAccountResponse>& responses,
//                            const size_t& total_pmidmgrs,
//                            const size_t& pmidmagsr_with_account);
//  void UpdateLocalStorage(const std::map<DataNameVariant, uint16_t>& expected_files);
//  void ApplyUpdateLocalStorage(const std::vector<DataNameVariant>& to_be_deleted,
//                               const std::vector<DataNameVariant>& to_be_retrieved);
//  std::vector<DataNameVariant> StoredFileNames();

  std::future<std::unique_ptr<ImmutableData>>
  RetrieveFileFromNetwork(const DataNameVariant& file_id);
  void HandleAccountResponses(
      const std::vector<nfs::GetPmidAccountResponseFromPmidManagerToPmidNode>& responses);
  template <typename Data>
  void HandlePut(const Data& data, const nfs::MessageId& message_id);
  template<typename Data>
  void HandleDelete(const typename Data::Name& name, const nfs::MessageId& message_id);

// ================================ Sender Validation =========================================
  template<typename T>
  bool ValidateSender(const T& /*message*/, const typename T::Sender& /*sender*/) const {
    return true;
  }

// ===================================Cache=====================================================
  template<typename T>
  bool DoGetFromCache(const T& message,
                      const typename T::Sender& sender,
                      const typename T::Receiver& receiver);

  template<typename T>
  bool CacheGet(const T& message,
                const typename T::Sender& sender,
                const typename T::Receiver& receiver,
                IsShortTermCacheable);

  template<typename T>
  bool CacheGet(const T& message,
                const typename T::Sender& sender,
                const typename T::Receiver& receiver,
                IsLongTermCacheable);

  template<typename T>
  void CacheStore(const T& message, const DataNameVariant& data_name, IsShortTermCacheable);

  template<typename T>
  void CacheStore(const T& message, const DataNameVariant& data_name, IsLongTermCacheable);

  template <typename T>
  void SendCachedData(const T& message,
                      const typename T::Sender& sender,
                      const typename T::Receiver& receiver,
                      const std::shared_ptr<NonEmptyString> content);

  routing::Routing& routing_;
  std::mutex accumulator_mutex_;
  Accumulator<nfs::PmidNodeServiceMessages> accumulator_;
  PmidNodeDispatcher dispatcher_;
  PmidNodeHandler handler_;
  Active active_;
  AsioService asio_service_;
  nfs_client::DataGetter data_getter_;
};

template<>
void PmidNodeService::HandleMessage(
    const nfs::PutRequestFromPmidManagerToPmidNode& message,
    const typename nfs::PutRequestFromPmidManagerToPmidNode::Sender& sender,
    const typename nfs::PutRequestFromPmidManagerToPmidNode::Receiver& receiver);


//template<>
//void PmidNodeService::HandleMessage<nfs::GetRequestFromDataManagerToPmidNode>(
//    const nfs::GetRequestFromDataManagerToPmidNode& message,
//    const typename nfs::GetRequestFromDataManagerToPmidNode::Sender& sender,
//    const typename nfs::GetRequestFromDataManagerToPmidNode::Receiver& receiver);

//template<>
//void PmidNodeService::HandleMessage<nfs::DeleteRequestFromPmidManagerToPmidNode>(
//    const nfs::DeleteRequestFromPmidManagerToPmidNode& message,
//    const typename nfs::DeleteRequestFromPmidManagerToPmidNode::Sender& sender,
//    const typename nfs::DeleteRequestFromPmidManagerToPmidNode::Receiver& receiver);


//template<>
//void PmidNodeService::HandleMessage<nfs::GetPmidAccountResponseFromPmidManagerToPmidNode>(
//    const nfs::GetPmidAccountResponseFromPmidManagerToPmidNode& message,
//    const typename nfs::GetPmidAccountResponseFromPmidManagerToPmidNode::Sender& sender,
//    const typename nfs::GetPmidAccountResponseFromPmidManagerToPmidNode::Receiver& receiver);

//template<>
//bool PmidNodeService::GetFromCache<nfs::GetRequestFromMaidNodeToDataManager>(
//    const nfs::GetRequestFromMaidNodeToDataManager& message,
//    const typename nfs::GetRequestFromMaidNodeToDataManager::Sender& sender,
//    const typename nfs::GetRequestFromMaidNodeToDataManager::Receiver& receiver);

//template<>
//bool PmidNodeService::GetFromCache<nfs::GetRequestFromPmidNodeToDataManager>(
//    const nfs::GetRequestFromPmidNodeToDataManager& message,
//    const typename nfs::GetRequestFromPmidNodeToDataManager::Sender& sender,
//    const typename nfs::GetRequestFromPmidNodeToDataManager::Receiver& receiver);

//template<>
//void PmidNodeService::StoreInCache<nfs::GetResponseFromDataManagerToMaidNode>(
//    const nfs::GetResponseFromDataManagerToMaidNode& message,
//    const typename nfs::GetResponseFromDataManagerToMaidNode::Sender& sender,
//    const typename nfs::GetResponseFromDataManagerToMaidNode::Receiver& receiver);


// ============================== Put implementation =============================================

template<typename T>
void PmidNodeService::HandleMessage(const T& /*message*/,
                                    const typename T::Sender& /*sender*/,
                                    const typename T::Receiver& /*receiver*/) {}


template<typename Data>
void PmidNodeService::HandlePut(const Data& data, const nfs::MessageId& message_id) {
  try {
    handler_.PutToPermanentStore(data);
    dispatcher_.SendPutRespnse(data, message_id, make_error_code(CommonErrors::success));
  } catch(const maidsafe_error& error) {
    dispatcher_.SendPutRespnse(data, message_id, error);
  }
}

//template<>
//void PmidNodeService::HandleMessage<nfs::GetRequestFromDataManagerToPmidNode>(
//    const nfs::GetRequestFromDataManagerToPmidNode& message,
//    const typename nfs::GetRequestFromDataManagerToPmidNode::Sender& sender,
//    const typename nfs::GetRequestFromDataManagerToPmidNode::Receiver& receiver) {
//  typedef nfs::GetRequestFromDataManagerToPmidNode MessageType;
//  OperationHandlerWrapper<PmidNodeService, MessageType, nfs::PmidNodeServiceMessages>(
//      accumulator_,
//      [this](const MessageType& message, const typename MessageType::Sender& sender) {
//        return this->ValidateSender(message, sender);
//      },
//      Accumulator<nfs::PmidNodeServiceMessages>::AddRequestChecker(RequiredRequests(message)),
//      this,
//      accumulator_mutex_)(message, sender, receiver);
//}

//template<>
//void PmidNodeService::HandleMessage<nfs::DeleteRequestFromPmidManagerToPmidNode>(
//    const nfs::DeleteRequestFromPmidManagerToPmidNode& message,
//    const typename nfs::DeleteRequestFromPmidManagerToPmidNode::Sender& sender,
//    const typename nfs::DeleteRequestFromPmidManagerToPmidNode::Receiver& receiver) {
//  typedef nfs::DeleteRequestFromPmidManagerToPmidNode MessageType;
//  OperationHandlerWrapper<PmidNodeService, MessageType, nfs::PmidNodeServiceMessages>(
//      accumulator_,
//      [this](const MessageType& message, const typename MessageType::Sender& sender) {
//        return this->ValidateSender(message, sender);
//      },
//      Accumulator<nfs::PmidNodeServiceMessages>::AddRequestChecker(RequiredRequests(message)),
//      this,
//      accumulator_mutex_)(message, sender, receiver);
//}

template<>
void PmidNodeService::HandleMessage(
    const nfs::GetPmidAccountResponseFromPmidManagerToPmidNode& message,
    const typename nfs::GetPmidAccountResponseFromPmidManagerToPmidNode::Sender& sender,
    const typename nfs::GetPmidAccountResponseFromPmidManagerToPmidNode::Receiver& receiver);

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
      handler_.DeleteFromPermanentStore(nfs_vault::DataName(name.type, name.raw_name));
      // accumulator_.SetHandled(message, sender); To be moved to OperationWrapper
    }
  } catch(const std::exception& /*ex*/) {
    std::lock_guard<std::mutex> lock(accumulator_mutex_);
    // accumulator_.SetHandled(message, sender); To be moved to OperationWrapper
  }
}


//template<>
//bool PmidNodeService::GetFromCache<nfs::GetRequestFromMaidNodeToDataManager>(
//    const nfs::GetRequestFromMaidNodeToDataManager& message,
//    const typename nfs::GetRequestFromMaidNodeToDataManager::Sender& sender,
//    const typename nfs::GetRequestFromMaidNodeToDataManager::Receiver& receiver) {
//  return DoGetFromCache(message, sender, receiver);
//}

//template<>
//bool PmidNodeService::GetFromCache<nfs::GetRequestFromPmidNodeToDataManager>(
//    const nfs::GetRequestFromPmidNodeToDataManager& message,
//    const typename nfs::GetRequestFromPmidNodeToDataManager::Sender& sender,
//    const typename nfs::GetRequestFromPmidNodeToDataManager::Receiver& receiver) {
//  return DoGetFromCache(message, sender, receiver);
//}

//template<typename T>
//bool PmidNodeService::DoGetFromCache(const T& message,
//                                     const typename T::Sender& sender,
//                                     const typename T::Receiver& receiver) {
//  auto data_name(GetDataNameVariant(message.contents->type, message.contents->raw_name));
//  if (!boost::apply_visitor(CacheableVisitor(), data_name))
//    return false;
//  if (boost::apply_visitor(LongTermCacheableVisitor(), data_name))
//    return CacheGet(message, sender, receiver, IsLongTermCacheable());
//  return CacheGet(message, sender, receiver, IsShortTermCacheable());
//}

//template<typename T>
//bool PmidNodeService::CacheGet(const T& message,
//                               const typename T::Sender& sender,
//                               const typename T::Receiver& receiver,
//                               IsShortTermCacheable) {
//  try {
//    auto content(std::make_shared<NonEmptyString>(handler_.mem_only_cache_.Get(*message.contents)));
//    active_.Send([=]() {
//                   SendCachedData(message, sender, receiver, content);
//                 });
//  } catch(maidsafe_error& /*error*/) {
//    return false;
//  }
//  return true;
//}

//template<typename T>
//bool PmidNodeService::CacheGet(const T& message,
//                               const typename T::Sender& sender,
//                               const typename T::Receiver& receiver,
//                               IsLongTermCacheable) {
//  try {
//    auto content(std::make_shared<NonEmptyString>(handler_.cache_data_store_.Get(*message.contents)));
//    active_.Send([=]() {
//                   SendCachedData(message, sender, receiver, content);
//                 });
//  } catch(maidsafe_error& /*error*/) {
//    return false;
//  }
//  return true;
//}

//template<>
//void PmidNodeService::StoreInCache<nfs::GetResponseFromDataManagerToMaidNode>(
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

//template<typename T>
//void PmidNodeService::CacheStore(const T& message,
//                                 const DataNameVariant& data_name,
//                                 IsShortTermCacheable) {
//  handler_.mem_only_cache_.Store(data_name, message.contents->data->content);
//}

//template<typename T>
//void PmidNodeService::CacheStore(const T& message,
//                                 const DataNameVariant& name,
//                                 IsLongTermCacheable) {
//  handler_.cache_data_store_.Store(name, message.contents->data->content);
//}

//template <typename T>
//void PmidNodeService::SendCachedData(const T& message,
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

#endif  // MAIDSAFE_VAULT_PMID_NODE_SERVICE_H_
