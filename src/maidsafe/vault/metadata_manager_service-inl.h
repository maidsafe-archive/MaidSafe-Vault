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

#ifndef MAIDSAFE_VAULT_METADATA_MANAGER_SERVICE_INL_H_
#define MAIDSAFE_VAULT_METADATA_MANAGER_SERVICE_INL_H_

#include <exception>
#include <string>
#include <vector>

#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/vault/utils.h"


namespace maidsafe {

namespace vault {

template<typename Data>
void MetadataManagerService::HandleDataMessage(const nfs::DataMessage& /*data_message*/,
                                        const routing::ReplyFunctor& /*reply_functor*/) {
}

template<typename Data>
void MetadataManagerService::HandlePut(const nfs::DataMessage& data_message,
                                       const routing::ReplyFunctor& reply_functor) {
  if (routing::GroupRangeStatus::kOutwithRange ==
      routing_.IsNodeIdInGroupRange(data_message.source().node_id)) {
    reply_functor(nfs::Reply(RoutingErrors::not_in_range).Serialise()->string());
    return;
  }

  // if (request_queue_.Push(message.id(), message.name())) {
  //   nfs::OnError on_error_callback = [this] (nfs::Message message) {
  //                                      this->OnPutErrorHandler<Data>(message);
  //                                    };
  //   nfs_.Put<Data>(message, on_error_callback);
  // }
  reply_functor(nfs::Reply(0).Serialise()->string());
}

template<typename Data>
void MetadataManagerService::HandleGet(nfs::DataMessage /*data_message*/,
                                       const routing::ReplyFunctor& /*reply_functor*/) {
//  std::vector<Identity> online_dataholders(
//      data_elements_manager_.GetOnlinePmid(Identity(data_message.data().name)));
//  std::vector<std::future<Data>> futures;
//  for (auto& online_dataholder : online_dataholders)
//    futures.emplace_back(nfs_.Get<Data>(data_message.data().name,
//                                        nfs::MessageSource(nfs::Persona::kMetadataManager,
//                                                           routing_.kNodeId()),
//                                        online_dataholder));

//  auto fetched_data = futures.begin();
//  while (futures.size() > 0) {
//    if (fetched_data->wait_for(0) == std::future_status::ready) {
//      try {
//        Data fetched_chunk = fetched_data->get();
//        if (fetched_chunk.name().data.IsInitialised()) {
//          reply_functor(fetched_chunk.data.string());
//          return;
//        }
//      } catch(...) {
//        // no op
//      }
//      fetched_data = futures.erase(fetched_data);
//    } else {
//      ++fetched_data;
//    }
//    Sleep(boost::posix_time::milliseconds(1));
//    if (fetched_data == futures.end())
//      fetched_data = futures.begin();
//  }

//  reply_functor(nfs::Reply(NfsErrors::failed_to_get_data).Serialise()->string());
}

template<typename Data>
void MetadataManagerService::HandleDelete(const nfs::DataMessage& /*data_message*/,
                                          const routing::ReplyFunctor& /*reply_functor*/) {
//  Identity data_id(data_message.data().name);
//  int64_t num_follower(data_elements_manager_.DecreaseDataElement(data_id));
//  if (num_follower == 0) {
//    std::vector<Identity> online_dataholders(data_elements_manager_.GetOnlinePmid(data_id));
//    data_elements_manager_.RemoveDataElement(data_id);

//    for (auto& online_dataholder : online_dataholders) {
//      nfs::DataMessage::OnError on_error_callback(
//          [this](nfs::DataMessage data_msg) { this->OnDeleteErrorHandler<Data>(data_msg); });
//      // TODO(Team) : double check whether signing key required
//      nfs::DataMessage new_message(
//          nfs::DataMessage::Action::kDelete,
//          nfs::Persona::kPmidAccountHolder,
//          nfs::MessageSource(nfs::Persona::kMetadataManager, routing_.kNodeId()),
//          nfs::DataMessage::Data(Data::name_type::tag_type::kEnumValue,
//                                 online_dataholder,
//                                 data_id));
//      nfs_.Delete<Data>(new_message, on_error_callback);
//    }
//  }
//  reply_functor(nfs::Reply(num_follower).Serialise()->string());
}

// On error handler's
template<typename Data>
void MetadataManagerService::OnPutErrorHandler(nfs::DataMessage data_message) {
  if (routing::GroupRangeStatus::kInRange ==
      routing_.IsNodeIdInGroupRange(data_message.source().node_id)) {
    nfs_.Put<Data>(data_message,
         [this](nfs::DataMessage data_msg) { this->OnPutErrorHandler<Data>(data_msg); });
  }
}

template<typename Data>
void MetadataManagerService::OnDeleteErrorHandler(nfs::DataMessage data_message) {
  if (routing::GroupRangeStatus::kInRange ==
      routing_.IsNodeIdInGroupRange(data_message.source().node_id)) {
    nfs_.Delete<Data>(data_message,
        [this](nfs::DataMessage data_msg) { this->OnDeleteErrorHandler<Data>(data_msg); });
  }
}

template<typename Data>
void MetadataManagerService::OnGenericErrorHandler(nfs::GenericMessage /*generic_message*/) {}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_METADATA_MANAGER_SERVICE_INL_H_
