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
#include <string>

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
      nfs::Reply reply(request.return_code);
      handled_request->set_reply(reply.Serialise()->string());
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
      nfs::Reply reply(nfs::Reply::serialised_type(NonEmptyString(
          proto_handled_requests.handled_requests(index).reply())));
      handled_requests.push_back(
          HandledRequest(
              nfs::MessageId(Identity(proto_handled_requests.handled_requests(index).message_id())),
              passport::PublicMaid::name_type(Identity(proto_handled_requests.name())),
              static_cast<nfs::DataMessage::Action>(
                  proto_handled_requests.handled_requests(index).action()),
              Identity(proto_handled_requests.handled_requests(index).data_name()),
              static_cast<DataTagValue>(proto_handled_requests.handled_requests(index).data_type()),
              proto_handled_requests.handled_requests(index).size(),
              reply.error()));
    }
  }
  catch(const std::exception&) {
    ThrowError(CommonErrors::parsing_error);
  }
  return handled_requests;
}

}  // namespace vault

}  // namespace maidsafe
