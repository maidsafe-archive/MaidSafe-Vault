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

#include "maidsafe/vault/pmid_node/service.h"

#include <chrono>
#include <limits>
#include <map>
#include <string>

#include "maidsafe/common/types.h"
#include "maidsafe/common/data_buffer.h"
#include "maidsafe/nfs/client/messages.pb.h"

#include "maidsafe/vault/pmid_manager/pmid_manager.pb.h"
#include "maidsafe/vault/operation_handlers.h"

namespace fs = boost::filesystem;

namespace maidsafe {

namespace vault {

namespace {

// inline bool SenderIsConnectedVault(const nfs::Message& message, routing::Routing& routing) {
//  return routing.IsConnectedVault(message.source().node_id) &&
//         routing.EstimateInGroup(message.source().node_id, routing.kNodeId());
// }

// inline bool SenderInGroupForMetadata(const nfs::Message& message, routing::Routing& routing) {
//  return routing.EstimateInGroup(message.source().node_id, NodeId(message.data().name.string()));
// }

template <typename Message>
inline bool ForThisPersona(const Message& message) {
  return message.destination_persona() != nfs::Persona::kPmidNode;
}

}  // unnamed namespace

PmidNodeService::PmidNodeService(const passport::Pmid& /*pmid*/, routing::Routing& routing,
                                 nfs_client::DataGetter& data_getter,
                                 const fs::path& vault_root_dir, DiskUsage max_disk_usage)
    : routing_(routing),
      accumulator_mutex_(),
#ifdef USE_MAL_BEHAVIOUR
      malfunc_behaviour_seed_(RandomUint32()),
#endif
      dispatcher_(routing_),
      handler_(vault_root_dir, max_disk_usage),
      active_(),
      data_getter_(data_getter) {
  StartUp();
  //  nfs_.GetElementList();  // TODO (Fraser) BEFORE_RELEASE Implementation needed
}

template <>
void PmidNodeService::HandleMessage(
    const PutRequestFromPmidManagerToPmidNode& message,
    const typename PutRequestFromPmidManagerToPmidNode::Sender& sender,
    const typename PutRequestFromPmidManagerToPmidNode::Receiver& receiver) {
  LOG(kVerbose) << message;
  typedef PutRequestFromPmidManagerToPmidNode MessageType;
  OperationHandlerWrapper<PmidNodeService, MessageType>(
      accumulator_, [this](const MessageType & message, const MessageType::Sender & sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)), this,
      accumulator_mutex_)(message, sender, receiver);
}

template<>
void PmidNodeService::HandleMessage(
    const GetRequestFromDataManagerToPmidNode& message,
    const typename GetRequestFromDataManagerToPmidNode::Sender& sender,
    const typename GetRequestFromDataManagerToPmidNode::Receiver& receiver) {
  LOG(kVerbose) << message;
  typedef GetRequestFromDataManagerToPmidNode MessageType;
  OperationHandlerWrapper<PmidNodeService, MessageType>(
      accumulator_, [this](const MessageType & message, const MessageType::Sender & sender) {
                        return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)), this,
      accumulator_mutex_)(message, sender, receiver);
}

template <>
void PmidNodeService::HandleMessage(
    const IntegrityCheckRequestFromDataManagerToPmidNode& message,
    const typename IntegrityCheckRequestFromDataManagerToPmidNode::Sender& sender,
    const typename IntegrityCheckRequestFromDataManagerToPmidNode::Receiver& receiver) {
  LOG(kVerbose) << message;
  typedef IntegrityCheckRequestFromDataManagerToPmidNode MessageType;
  OperationHandlerWrapper<PmidNodeService, MessageType>(
      accumulator_, [this](const MessageType & message, const MessageType::Sender & sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)), this,
      accumulator_mutex_)(message, sender, receiver);
}

template<>
void PmidNodeService::HandleMessage(
  const DeleteRequestFromPmidManagerToPmidNode& message,
  const typename DeleteRequestFromPmidManagerToPmidNode::Sender& sender,
  const typename DeleteRequestFromPmidManagerToPmidNode::Receiver& receiver) {
  LOG(kVerbose) << message;
  typedef DeleteRequestFromPmidManagerToPmidNode MessageType;
  OperationHandlerWrapper<PmidNodeService, MessageType>(
      accumulator_, [this](const MessageType & message, const MessageType::Sender & sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)), this,
      accumulator_mutex_)(message, sender, receiver);
}

void PmidNodeService::StartUp() {
//  dispatcher_.SendPmidAccountRequest(handler_.AvailableSpace());
}

void PmidNodeService::UpdateLocalStorage(const std::vector<DataNameVariant>& to_be_deleted,
                                         const std::vector<DataNameVariant>& /*to_be_retrieved*/) {
  for (auto file_name : to_be_deleted) {
    try {
      handler_.Delete(file_name);
    }
    catch(const maidsafe_error& error) {
      LOG(kWarning) << "Error in deletion: " << error.code() << " - "
                    << boost::diagnostic_information(error);
    }
  }

  std::vector<std::future<void>> futures;
//  for (auto file_name : to_be_retrieved) {
//    GetCallerVisitor get_caller_visitor(data_getter_,
//                                        futures,
//                                        [this](const DataNameVariant& key,
//                                               const NonEmptyString& value) {
//                                          this->handler_.Put(key, value);
//                                        });
//    boost::apply_visitor(get_caller_visitor, file_name);
//  }

  for (auto iter(futures.begin()); iter != futures.end(); ++iter) {
    try {
      iter->wait();
    }
    catch(const maidsafe_error& error) {
      LOG(kWarning) << "Error in retreivel: " << error.code() << " - "
                    << boost::diagnostic_information(error);
    }
  }
}

}  // namespace vault

}  // namespace maidsafe
