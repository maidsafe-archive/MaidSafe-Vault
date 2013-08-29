/* Copyright 2012 MaidSafe.net limited

This MaidSafe Software is licensed under the MaidSafe.net Commercial License, version 1.0 or later,
and The General Public License (GPL), version 3. By contributing code to this project You agree to
the terms laid out in the MaidSafe Contributor Agreement, version 1.0, found in the root directory
of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available at:

http://www.novinet.com/license

Unless required by applicable law or agreed to in writing, software distributed under the License is
distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.
*/

#ifndef  MAIDSAFE_VAULT_ACCUMULATOR_INL_H_
#define  MAIDSAFE_VAULT_ACCUMULATOR_INL_H_

#include <algorithm>
#include <deque>
#include <iterator>
#include <string>
#include <vector>
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/data_types/data_name_variant.h"

#include "maidsafe/vault/handled_request.pb.h"
#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

template<typename T>
Accumulator<T>::Accumulator()
    : pending_requests_(),
      handled_requests_(),
      kMaxPendingRequestsCount_(300),
      kMaxHandledRequestsCount_(1000) {}

template<typename T>
typename Accumulator<T>::AddResult Accumulator<T>::AddPendingRequest(
    const T& request,
    const routing::GroupSource& source,
    AddRequestPredicate predicate) {
  if (CheckHandled(request))
    return false;
  bool already_exists(false);
  auto request_message_id(boost::apply_visitor(message_id_requestor_visitor(), request));
  nfs::MessageId message_id;
  for (auto pending_request : pending_requests_) {
    if (pending_request.request.which() == request.which()) {
      message_id = boost::apply_visitor(message_id_requestor_visitor(), pending_request.request);
      if (message_id == request_message_id) {
        if (source == pending_request.source)
          already_exists = true;
      }
    }
  }
  if (!already_exists) {
    pending_requests_.push_back(PendingRequest(request, source));
    if (pending_requests_.size() > kMaxPendingRequestsCount_)
      handled_requests_.pop_front();
  }
  return predicate(Get(request));
}

template<typename T>
bool Accumulator<T>::CheckHandled(const T& request) {
  auto request_message_id(boost::apply_visitor(message_id_requestor_visitor(), request));
  nfs::MessageId message_id;
  for (auto handled_request : handled_requests_) {
    if (handled_request.which() == request.which()) {
      message_id = boost::apply_visitor(message_id_requestor_visitor(), handled_request);
      if (message_id == request_message_id)
        return true;
    }
  }
  return false;
}

template<typename T>
void Accumulator<T>::SetHandled(const T& request, const routing::GroupSource& source) {
  assert(!CheckHandled(request) && "Request has already been set as handled");
  std::set<int> pending_requests_to_remove;
  nfs::MessageId message_id;
  auto request_message_id(boost::apply_visitor(message_id_requestor_visitor(), request));
  boost::apply_visitor(content_eraser_visitor(), request);
  for (uint16_t index(0); index < pending_requests_.size(); ++index) {
    if (pending_requests_.at(index).request.which() == request.which()) {
      message_id = boost::apply_visitor(message_id_requestor_visitor(),
                                        pending_requests_.at(index).request);
      if ((message_id == request_message_id) &&
          (source.group_id == pending_requests_.at(index).source.group_id))
        pending_requests_to_remove.insert(index);
    }
  }
  boost::apply_visitor(content_eraser_visitor(), request);
  handled_requests_.push_back(request);
  size_t size_to_remove(pending_requests_to_remove.size());
  for (size_t index(0); index < size_to_remove; ++index) {
    auto iterator(pending_requests_.begin());
    std::advance(iterator, *pending_requests_to_remove.begin() - index);
    pending_requests_.erase(iterator);
    pending_requests_to_remove.erase(pending_requests_to_remove.begin());
  }
  if (handled_requests_.size() > kMaxHandledRequestsCount_)
    handled_requests_.pop_front();
}

template<typename T>
std::vector<T> Accumulator<T>::Get(const T& request) {
  std::vector<T> requests;
  nfs::MessageId message_id;
  auto request_message_id(boost::apply_visitor(message_id_requestor_visitor(), request));
  for (auto pending_request : pending_requests_) {
    if (pending_request.request.which() == request.which()) {
      message_id = boost::apply_visitor(message_id_requestor_visitor(), pending_request.request);
      if ((message_id == request_message_id))
        requests.insert(pending_request.request);
    }
  }
  return std::move(requests);
}


//template<typename Name>
//typename std::deque<typename Accumulator<Name>::HandledRequest>::const_iterator
//    Accumulator<Name>::FindHandled(const nfs::Message& message) const {
//  return std::find_if(std::begin(handled_requests_),
//                      std::end(handled_requests_),
//                      [&message](const HandledRequest& handled_request) {
//                      return (handled_request.msg_id == message.message_id()) &&
//                             (handled_request.account_name ==
//                                 Name(Identity(message.source().node_id.string())));
//                      });
//}

//template<typename Name>
//bool Accumulator<Name>::CheckHandled(const nfs::Message& message, nfs::Reply& reply_out) const {
//  const auto it(FindHandled(message));  // NOLINT (dirvine)
//  if (it != std::end(handled_requests_)) {
//    if (it->reply.error().code() == CommonErrors::success)
//      reply_out = it->reply;
//    else
//      reply_out = nfs::Reply(it->reply.error(), message.Serialise().data);
//    return true;
//  }
//  return false;
//}

//template<typename Name>
//std::vector<nfs::Reply> Accumulator<Name>::PushSingleResult(
//    const nfs::Message& message,
//    const routing::ReplyFunctor& reply_functor,
//    const nfs::Reply& reply) {
//  std::vector<nfs::Reply> replies;
//  if (FindHandled(message) != std::end(handled_requests_))
//    return replies;

//  PendingRequest pending_request(message, reply_functor, reply);
//  pending_requests_.push_back(pending_request);
//  for (auto& request : pending_requests_) {
//    if (request.msg.message_id() == message.message_id() &&
//        request.msg.source().node_id == message.source().node_id) {
//      replies.emplace_back(request.reply);
//    }
//  }
//  // TODO(Profiling) - Consider holding requests in a map/set and each having a timestamp.
//  // Pruning could then be done periodically (event-based) and would allow sorted list of requests.
//  if (pending_requests_.size() > kMaxPendingRequestsCount_)
//    pending_requests_.pop_front();
//  return replies;
//}

//template<typename Name>
//std::vector<nfs::Reply> Accumulator<Name>::GetPendingResults(const nfs::Message& message) const {
//  std::vector<nfs::Reply> replies;
//  if (FindHandled(message) != std::end(handled_requests_))
//    return replies;

//  for (auto& request : pending_requests_) {
//    if (request.msg.message_id() == message.message_id() &&
//        request.msg.source().node_id == message.source().node_id) {
//      replies.emplace_back(request.reply);
//    }
//  }
//  return replies;
//}

//template<typename Name>
//std::vector<typename Accumulator<Name>::PendingRequest> Accumulator<Name>::SetHandled(
//    const nfs::Message& message,
//    const nfs::Reply& reply) {
//  std::vector<PendingRequest> ret_requests;
//  auto itr = pending_requests_.begin();
//  while (itr != pending_requests_.end()) {
//    if ((*itr).msg.message_id() == message.message_id() &&
//        (*itr).msg.source().node_id == message.source().node_id) {
//      ret_requests.push_back(*itr);
//      itr = pending_requests_.erase(itr);
//    } else {
//      ++itr;
//    }
//  }

//  handled_requests_.push_back(
//      Accumulator::HandledRequest(message.message_id(),
//                                  Name(Identity(message.source().node_id.string())),
//                                  message.data().action,
//                                  message.data().name,
//                                  *message.data().type,
//                                  static_cast<int32_t>(message.data().content.string().size()),
//                                  reply));
//  if (handled_requests_.size() > kMaxHandledRequestsCount_)
//    handled_requests_.pop_front();
//  return ret_requests;
//}

//template<typename Name>
//void Accumulator<Name>::SetHandledAndReply(const nfs::MessageId message_id,
//                                           const NodeId& source_node,
//                                           const nfs::Reply& reply_out) {
//  auto return_code(CommonErrors::success);
//  nfs::Reply reply(return_code);
//  std::vector<PendingRequest> ret_requests;
//  auto itr = pending_requests_.begin();
//  while (itr != pending_requests_.end()) {
//    if ((*itr).msg.message_id() == message_id &&
//        (*itr).msg.source().node_id == source_node) {
//      ret_requests.push_back(*itr);
//      itr = pending_requests_.erase(itr);
//    } else {
//      ++itr;
//    }
//  }

//  handled_requests_.push_back(
//      Accumulator::HandledRequest(message_id,
//                                  Name(Identity(source_node.string())),
//                                  reply_out));
//  if (handled_requests_.size() > kMaxHandledRequestsCount_)
//    handled_requests_.pop_front();
//  // send replies if required
//  for (const auto& ret : ret_requests) {
//    if (ret.reply_functor)
//      ret.reply_functor(reply_out.Serialise()->string());
//  }
//}



// Workaround for gcc 4.6 bug related to warning "redundant redeclaration" for template
// specialisation. refer // http://gcc.gnu.org/bugzilla/show_bug.cgi?id=15867#c4

//#ifdef __GNUC__
//#  pragma GCC diagnostic push
//#  pragma GCC diagnostic ignored "-Wredundant-decls"
//#endif
//template<>
//typename std::deque<typename Accumulator<DataNameVariant>::HandledRequest>::const_iterator
//    Accumulator<DataNameVariant>::FindHandled(const nfs::Message& message) const;

//template<>
//std::vector<typename Accumulator<DataNameVariant>::PendingRequest>
//    Accumulator<DataNameVariant>::SetHandled(
//        const nfs::Message& message,
//        const nfs::Reply& reply);

//template<>
//std::vector<typename Accumulator<PmidName>::PendingRequest> Accumulator<PmidName>::SetHandled(
//    const nfs::Message& message,
//    const nfs::Reply& reply);
//#ifdef __GNUC__
//#  pragma GCC diagnostic pop
//#endif


}  // namespace vault

}  // namespace maidsafe


#endif  // MAIDSAFE_VAULT_ACCUMULATOR_INL_H_
