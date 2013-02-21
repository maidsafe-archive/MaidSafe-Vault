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

#ifndef  MAIDSAFE_VAULT_ACCUMULATOR_INL_H_
#define  MAIDSAFE_VAULT_ACCUMULATOR_INL_H_

#include <iterator>
#include <string>
#include <vector>
#include <deque>

#include "maidsafe/nfs/reply.h"


namespace maidsafe {

namespace vault {

template<typename Name>
Accumulator<Name>::PendingRequest::PendingRequest(const nfs::DataMessage& msg_in,
                                                  const routing::ReplyFunctor& reply_functor_in,
                                                  const maidsafe_error& return_code_in)
    : msg(msg_in),
      reply_functor(reply_functor_in),
      return_code(return_code_in) {}

template<typename Name>
Accumulator<Name>::PendingRequest::PendingRequest(const PendingRequest& other)
    : msg(other.msg),
      reply_functor(other.reply_functor),
      return_code(other.return_code) {}

template<typename Name>
typename Accumulator<Name>::PendingRequest& Accumulator<Name>::PendingRequest::operator=(
    const PendingRequest& other) {
  msg = other.msg;
  reply_functor = other.reply_functor;
  return_code = other.return_code;
  return *this;
}

template<typename Name>
Accumulator<Name>::PendingRequest::PendingRequest(PendingRequest&& other)
    : msg(std::move(other.msg)),
      reply_functor(std::move(other.reply_functor)),
      return_code(std::move(other.return_code)) {}

template<typename Name>
typename Accumulator<Name>::PendingRequest& Accumulator<Name>::PendingRequest::operator=(
    PendingRequest&& other) {
  msg = std::move(other.msg);
  reply_functor = std::move(other.reply_functor);
  return_code = std::move(other.return_code);
  return *this;
}

template<typename Name>
Accumulator<Name>::HandledRequest::HandledRequest(const nfs::MessageId& msg_id_in,
                                                  const Name& source_name_in,
                                                  const nfs::DataMessage::Action& action_type_in,
                                                  const Identity& data_name_in,
                                                  const DataTagValue& data_type_in,
                                                  const int32_t& size_in,
                                                  const maidsafe_error& return_code_in)
    : msg_id(msg_id_in),
      source_name(source_name_in),
      action(action_type_in),
      data_name(data_name_in),
      data_type(data_type_in),
      size(size_in),
      return_code(return_code_in) {}

template<typename Name>
Accumulator<Name>::HandledRequest::HandledRequest(const HandledRequest& other)
    : msg_id(other.msg_id),
      source_name(other.source_name),
      action(other.action),
      data_name(other.data_name),
      data_type(other.data_type),
      size(other.size),
      return_code(other.return_code) {}

template<typename Name>
typename Accumulator<Name>::HandledRequest& Accumulator<Name>::HandledRequest::operator=(
    const HandledRequest& other) {
  msg_id = other.msg_id;
  source_name = other.source_name;
  action = other.action;
  data_name = other.data_name,
  data_type = other.data_type,
  size = other.size;
  return_code = other.return_code;
  return *this;
}

template<typename Name>
Accumulator<Name>::HandledRequest::HandledRequest(HandledRequest&& other)
    : msg_id(std::move(other.msg_id)),
      source_name(std::move(other.source_name)),
      action(std::move(other.action)),
      data_name(std::move(other.data_name)),
      data_type(std::move(other.data_type)),
      size(std::move(other.size)),
      return_code(std::move(other.return_code)) {}

template<typename Name>
typename Accumulator<Name>::HandledRequest& Accumulator<Name>::HandledRequest::operator=(
    HandledRequest&& other) {
  msg_id = std::move(other.msg_id);
  source_name = std::move(other.source_name);
  action = std::move(other.action);
  data_name = std::move(other.data_name),
  data_type = std::move(data_type);
  size = std::move(other.size);
  return_code = std::move(other.return_code);
  return *this;
}

template<typename Name>
Accumulator<Name>::Accumulator()
    : pending_requests_(),
      handled_requests_(),
      kMaxPendingRequestsCount_(300),
      kMaxHandledRequestsCount_(1000) {}

template<typename Name>
typename std::deque<typename Accumulator<Name>::HandledRequest>::const_iterator
    Accumulator<Name>::FindHandled(const nfs::DataMessage& data_message) const {
  return std::find_if(std::begin(handled_requests_),
                      std::end(handled_requests_),
                      [&data_message](const HandledRequest& handled_request) {
                      return (handled_request.msg_id == data_message.message_id()) &&
                             (handled_request.source_name ==
                                 Name(Identity(data_message.source().node_id.string())));
                      });
}

template<typename Name>
bool Accumulator<Name>::CheckHandled(const nfs::DataMessage& data_message,
                                     nfs::Reply& reply_out) const {
  const auto it(FindHandled(data_message));  // NOLINT (dirvine)
  if (it != std::end(handled_requests_)) {
    if (it->return_code.code() == CommonErrors::success)
      reply_out = nfs::Reply(it->return_code);
    else
      reply_out = nfs::Reply(it->return_code, data_message.Serialise().data);
    return true;
  }
  return false;
}

template<typename Name>
std::vector<nfs::Reply> Accumulator<Name>::PushSingleResult(
    const nfs::DataMessage& data_message,
    const routing::ReplyFunctor& reply_functor,
    const maidsafe_error& return_code) {
  std::vector<nfs::Reply> replies;
  if (FindHandled(data_message) != std::end(handled_requests_))
    return replies;

  PendingRequest pending_request(data_message, reply_functor, return_code);
  pending_requests_.push_back(pending_request);
  for (auto& request : pending_requests_) {
    if (request.msg.message_id() == data_message.message_id() &&
        request.msg.source().node_id == data_message.source().node_id) {
      replies.push_back(request.return_code);
    }
  }
  if (pending_requests_.size() > kMaxPendingRequestsCount_)
    pending_requests_.pop_front();
  return replies;
}

template<typename Name>
std::vector<typename Accumulator<Name>::PendingRequest> Accumulator<Name>::SetHandled(
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
                                  Name(Identity(data_message.source().node_id.string())),
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


#endif  // MAIDSAFE_VAULT_ACCUMULATOR_INL_H_
