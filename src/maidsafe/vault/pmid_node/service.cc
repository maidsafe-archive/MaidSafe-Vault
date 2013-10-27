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

#include <string>
#include <chrono>

#include "maidsafe/common/types.h"
#include "maidsafe/data_store/data_buffer.h"
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
//}

// inline bool SenderInGroupForMetadata(const nfs::Message& message, routing::Routing& routing) {
//  return routing.EstimateInGroup(message.source().node_id, NodeId(message.data().name.string()));
//}

template <typename Message>
inline bool ForThisPersona(const Message& message) {
  return message.destination_persona() != nfs::Persona::kPmidNode;
}

}  // unnamed namespace

PmidNodeService::PmidNodeService(const passport::Pmid& /*pmid*/, routing::Routing& routing,
                                 nfs_client::DataGetter& data_getter,
                                 const fs::path& vault_root_dir)
    : routing_(routing),
      accumulator_mutex_(),
      dispatcher_(routing_),
      handler_(vault_root_dir),
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
    const GetRequestFromDataManagerToPmidNode& /*message*/,
    const typename GetRequestFromDataManagerToPmidNode::Sender& /*sender*/,
    const typename GetRequestFromDataManagerToPmidNode::Receiver& /*receiver*/) {
  // typedef GetRequestFromDataManagerToPmidNode MessageType;
  assert(0);
}

template <>
void PmidNodeService::HandleMessage(
    const IntegrityCheckRequestFromDataManagerToPmidNode& message,
    const typename IntegrityCheckRequestFromDataManagerToPmidNode::Sender& sender,
    const typename IntegrityCheckRequestFromDataManagerToPmidNode::Receiver& receiver) {
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
  typedef DeleteRequestFromPmidManagerToPmidNode MessageType;
  OperationHandlerWrapper<PmidNodeService, MessageType>(
      accumulator_, [this](const MessageType & message, const MessageType::Sender & sender) {
                      return this->ValidateSender(message, sender);
                    },
      Accumulator<Messages>::AddRequestChecker(RequiredRequests(message)), this,
      accumulator_mutex_)(message, sender, receiver);
}

template <>
void PmidNodeService::HandleMessage(
    const GetPmidAccountResponseFromPmidManagerToPmidNode& message,
    const typename GetPmidAccountResponseFromPmidManagerToPmidNode::Sender& sender,
    const typename GetPmidAccountResponseFromPmidManagerToPmidNode::Receiver& receiver) {
  typedef GetPmidAccountResponseFromPmidManagerToPmidNode MessageType;
  auto add_request_predicate(
      [&](const std::vector<Messages>& requests_in) {
        if (requests_in.size() < 2)
          return Accumulator<Messages>::AddResult::kWaiting;
        int  valid_response_size(0);
        for (auto& request : requests_in) {
          auto typed_request(boost::get<MessageType>(request));
          if (typed_request.contents->return_code.value.code() == CommonErrors::success)
            valid_response_size++;
        }
        if ((static_cast<uint16_t>(requests_in.size()) >=
                (routing::Parameters::node_group_size / 2 + 1U)) &&
            valid_response_size >= routing::Parameters::node_group_size / 2)
          return Accumulator<Messages>::AddResult::kSuccess;
          if ((requests_in.size() == routing::Parameters::node_group_size) ||
              (requests_in.size() - valid_response_size > routing::Parameters::node_group_size / 2))
              return Accumulator<Messages>::AddResult::kFailure;
            return Accumulator<Messages>::AddResult::kWaiting;
          });

  OperationHandlerWrapper<PmidNodeService, MessageType>(
      accumulator_, [this](const MessageType & message, const MessageType::Sender & sender) {
                      return this->ValidateSender(message, sender);
                    },
      add_request_predicate, this, accumulator_mutex_)(message, sender, receiver);
}

void PmidNodeService::HandlePmidAccountResponses(
    const std::vector<std::set<nfs_vault::DataName>>& responses, int failures) {
  const auto total_responses(responses.size() + failures);
  std::map<nfs_vault::DataName, uint16_t> chunks_expectation;
  std::vector<DataNameVariant> expected_chunks;
  for (auto data_names : responses) {
    for (const auto& data_name : data_names)
      chunks_expectation[data_name]++;
  }

  for (auto iter(chunks_expectation.begin()); iter != chunks_expectation.end();) {
    if ((iter->second >= routing::Parameters::node_group_size / 2 + 1U) ||
        ((iter->second == routing::Parameters::node_group_size / 2) &&
             (total_responses > responses.size())))
      expected_chunks.push_back(GetDataNameVariant(iter->first.type, iter->first.raw_name));
  }
  CheckPmidAccountResponsesStatus(expected_chunks);
}

void PmidNodeService::CheckPmidAccountResponsesStatus(
    const std::vector<DataNameVariant>& expected_chunks) {
  std::vector<DataNameVariant> all_data_names(handler_.GetAllDataNames());
  std::vector<DataNameVariant> to_be_deleted, to_be_retrieved;
  for (auto data_name : all_data_names) {
    if (std::any_of(std::begin(expected_chunks), std::end(expected_chunks),
                    [&data_name](const DataNameVariant& expected) {
                      return expected == data_name;
                    })) {
      to_be_deleted.push_back(data_name);
    }
  }
  for (auto iter(expected_chunks.begin()); iter != expected_chunks.end(); ++iter) {
    if (std::any_of(std::begin(all_data_names), std::end(all_data_names),
                    [&](const DataNameVariant& existing) { return existing == *iter; })) {
      to_be_retrieved.push_back(*iter);
    }
  }
  UpdateLocalStorage(to_be_deleted, to_be_retrieved);
}

void PmidNodeService::StartUp() {
  dispatcher_.SendPmidAccountRequest(handler_.AvailableSpace());
}

void PmidNodeService::UpdateLocalStorage(const std::vector<DataNameVariant>& to_be_deleted,
                                         const std::vector<DataNameVariant>& /*to_be_retrieved*/) {
  for (auto file_name : to_be_deleted) {
    try {
      handler_.Delete(file_name);
    }
    catch(const maidsafe_error& error) {
      LOG(kWarning) << "Error in deletion: " << error.code() << " - " << error.what();
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
      LOG(kWarning) << "Error in retreivel: " << error.code() << " - " << error.what();
    }
  }
}

}  // namespace vault

}  // namespace maidsafe
