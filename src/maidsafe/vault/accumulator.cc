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



namespace maidsafe {

namespace vault {

template<>
typename std::deque<typename Accumulator<DataNameVariant>::HandledRequest>::const_iterator
    Accumulator<DataNameVariant>::FindHandled(const nfs::DataMessage& data_message) const {
  return std::find_if(std::begin(handled_requests_),
                      std::end(handled_requests_),
                      [&data_message](const HandledRequest& handled_request)->bool {
                          auto req_name_and_type =
                              boost::apply_visitor(GetTagValueAndIdentityVisitor(),
                                                   handled_request.account_name);
                          return (handled_request.msg_id == data_message.message_id()) &&
                              (req_name_and_type.first == data_message.data().type) &&
                              (req_name_and_type.second.string() ==
                                   data_message.data().name.string());
                      });
}

template<>
std::vector<typename Accumulator<DataNameVariant>::PendingRequest>
    Accumulator<DataNameVariant>::SetHandled(
        const nfs::DataMessage& data_message,
        const maidsafe_error& return_code) {
  std::vector<PendingRequest> ret_requests;
  auto itr = pending_requests_.begin();
  while (itr != pending_requests_.end()) {
    if ((*itr).msg.message_id() == data_message.message_id() &&
        (*itr).msg.source().node_id == data_message.source().node_id) {
      ret_requests.push_back(*itr);
      itr = pending_requests_.erase(itr);
    } else {
      ++itr;
    }
  }

  handled_requests_.push_back(
      Accumulator::HandledRequest(data_message.message_id(),
                                  GetDataNameVariant(data_message.data().type, data_message.data().name),
                                  data_message.data().action,
                                  data_message.data().name,
                                  data_message.data().type,
                                  static_cast<int32_t>(data_message.data().content.string().size()),
                                  return_code));
  if (handled_requests_.size() > kMaxHandledRequestsCount_)
    handled_requests_.pop_front();
  return ret_requests;
}

template<>
std::vector<typename Accumulator<PmidName>::PendingRequest> Accumulator<PmidName>::SetHandled(
    const nfs::DataMessage& data_message,
    const maidsafe_error& return_code) {
  std::vector<PendingRequest> ret_requests;
  auto itr = pending_requests_.begin();
  while (itr != pending_requests_.end()) {
    if ((*itr).msg.message_id() == data_message.message_id() &&
        (*itr).msg.source().node_id == data_message.source().node_id) {
      ret_requests.push_back(*itr);
      itr = pending_requests_.erase(itr);
    } else {
      ++itr;
    }
  }

  handled_requests_.push_back(
      Accumulator::HandledRequest(data_message.message_id(),
                                  data_message.data_holder(),
                                  data_message.data().action,
                                  data_message.data().name,
                                  data_message.data().type,
                                  static_cast<int32_t>(data_message.data().content.string().size()),
                                  return_code));
  if (handled_requests_.size() > kMaxHandledRequestsCount_)
    handled_requests_.pop_front();
  return ret_requests;
}

}  // namespace vault

}  // namespace maidsafe
