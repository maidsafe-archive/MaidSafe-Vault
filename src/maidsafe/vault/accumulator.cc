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

#include "maidsafe/vault/accumulator.h"

#include <algorithm>

#include "maidsafe/vault/handled_request_pb.h"


namespace maidsafe {

namespace vault {

template<>
typename Accumulator<passport::PublicMaid::name_type>::serialised_requests
Accumulator<passport::PublicMaid::name_type>::Serialise(
    const passport::PublicMaid::name_type& name) const {
  protobuf::HandledRequests handled_requests;
  protobuf::HandledRequest* handled_request;
  handled_requests.set_name(name->string());
  for (auto& request : handled_requests_) {
    if (request.source_name == name) {
      handled_request = handled_requests.add_handled_requests();
      handled_request->set_message_id(request.msg_id->string());
      handled_request->set_action(static_cast<int32_t>(request.action));
      handled_request->set_data_name(request.data_name.string());
      handled_request->set_data_type(static_cast<int32_t>(request.data_type));
      handled_request->set_size(request.size);
      handled_request->set_reply(NonEmptyString(request.reply.Serialise()).string());
    }
  }
  return serialised_requests(NonEmptyString(handled_requests.SerializeAsString()));
}

template<>
std::vector<typename Accumulator<passport::PublicMaid::name_type>::HandledRequest>
Accumulator<passport::PublicMaid::name_type>::Parse(
    const typename Accumulator<passport::PublicMaid::name_type>::serialised_requests&
        serialised_requests_in) const {
  std::vector<typename Accumulator<passport::PublicMaid::name_type>::HandledRequest>
      handled_requests;
  protobuf::HandledRequests proto_handled_requests;
  if (!proto_handled_requests.ParseFromString(serialised_requests_in->string()))
    ThrowError(CommonErrors::parsing_error);
  try {
    for (auto index(0); index < proto_handled_requests.handled_requests_size(); ++index) {
      handled_requests.push_back(
          HandledRequest(
              nfs::MessageId(Identity(proto_handled_requests.handled_requests(index).message_id())),
              passport::PublicMaid::name_type(Identity(proto_handled_requests.name())),
              static_cast<nfs::DataMessage::Action>(
                  proto_handled_requests.handled_requests(index).action()),
              Identity(proto_handled_requests.handled_requests(index).data_name()),
              static_cast<DataTagValue>(proto_handled_requests.handled_requests(index).data_type()),
              proto_handled_requests.handled_requests(index).size(),
              nfs::Reply(nfs::Reply::serialised_type(NonEmptyString(
                  proto_handled_requests.handled_requests(index).reply())))));
    }
  }
  catch(const std::exception&) {
    ThrowError(CommonErrors::parsing_error);
  }
  return handled_requests;
}

// template<>
// void Accumulator<passport::PublicMaid::name_type>::HandleSyncUpdates(
//    const  NonEmptyString&  /*serialised_sync_updates*/)  {
//    auto sync_updates(ParseHandledRequests(serialised_requests(serialised_sync_updates)));
//  HandledRequests ready_to_update;
//  for (auto& sync_update : sync_updates) {
//    /* same update from same updater exist in pending_sync_updates_*/
//    if (std::find_if(pending_sync_updates_.begin(), pending_sync_updates_.end(),
//                     [&](const SyncData& sync) {
//                       return ((sync.updater_name == sync_update.updater_name) &&
//                               (sync.msg_id == sync_update.msg_id) &&
//                               (sync.data_name == sync_update.data_name));
//                     }) != pending_sync_updates_.end())
//      continue;
//    /* request is already handled and is in handled_requests_*/
//    if (std::find_if(handled_requests_.begin(), handled_requests_.end(),
//                     [&](const SyncData& sync) {
//                       return ((sync.updater_name == sync_update.updater_name) &&
//                               (sync.msg_id == sync_update.msg_id));
//                     }) != handled_requests_.end())
//      continue;
//    /* request is in pending_requests_ */
//    if (std::find_if(pending_requests_.begin(), pending_requests_.end(),
//                     [&](const PendingRequest& request) {
//                       return ((request.first.second == sync_update.source_name) &&
//                               (request.first.first == sync_update.msg_id));
//                     }) != pending_requests_.end())
//      continue;
//    /* no similar request exist add it to pending_sync_updates_*/
//    if (std::find_if(pending_sync_updates_.begin(), pending_sync_updates_.end(),
//                     [&](const SyncData& sync) {
//                       return ((sync.updater_name == sync_update.updater_name) &&
//                               (sync.msg_id == sync_update.msg_id));
//                     }) == pending_sync_updates_.end())
//      pending_sync_updates_.push_back(sync_update);
//    /* similar request from different updater exists in pending_sync_updates_
//     * if the number of similar requests is enough add the sync_update to ready_to_update list and
//     * remove the similar requests from pending_sync_updates_ list  */
//    auto iter(std::remove_if(pending_sync_updates_.begin(), pending_sync_updates_.end(),
//                             [&](const SyncData& sync)->bool {
//                               return ((sync.msg_id == sync_update.msg_id) &&
//                                 (sync.source_name == sync_update.source_name) &&
//                                 (sync.data_name == sync_update.data_name));
//                             }));
//    if (std::abs(std::distance(iter, pending_sync_updates_.end())) >= kMinResolutionCount_) {
//      ready_to_update.push_back(sync_update);
//      pending_sync_updates_.erase(iter, pending_sync_updates_.end());
//    }
//  }
// }

}  // namespace vault

}  // namespace maidsafe
