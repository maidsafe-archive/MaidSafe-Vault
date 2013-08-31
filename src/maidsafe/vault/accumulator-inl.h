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
#include <string>
#include <vector>

#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/vault/handled_request.pb.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/utils.h"


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
    AddCheckerFunctor checker) {
  if (CheckHandled(request))
    return Accumulator<T>::AddResult::kHandled;
  bool already_exists(false);
  auto request_message_id(boost::apply_visitor(MessageIdRequestVisitor(), request));
  nfs::MessageId message_id;
  for (auto pending_request : pending_requests_) {
    if (pending_request.request.which() == request.which()) {
      message_id = boost::apply_visitor(MessageIdRequestVisitor(), pending_request.request);
      if ((message_id == request_message_id) && (source == pending_request.source))
        already_exists = true;
    }
  }
  if (!already_exists) {
    pending_requests_.push_back(PendingRequest(request, source));
    if (pending_requests_.size() > kMaxPendingRequestsCount_)
      handled_requests_.pop_front();
  }
  return checker(Get(request));
}

template<typename T>
bool Accumulator<T>::CheckHandled(const T& request) {
  auto request_message_id(boost::apply_visitor(MessageIdRequestVisitor(), request));
  nfs::MessageId message_id;
  for (auto handled_request : handled_requests_) {
    if (handled_request.which() == request.which()) {
      message_id = boost::apply_visitor(MessageIdRequestVisitor(), handled_request);
      if (message_id == request_message_id)
        return true;
    }
  }
  return false;
}

template<typename T>
void Accumulator<T>::SetHandled(const T& request, const routing::GroupSource& source) {
  assert(!CheckHandled(request) && "Request has already been set as handled");
  nfs::MessageId message_id;
  auto request_message_id(boost::apply_visitor(MessageIdRequestVisitor(), request));
  boost::apply_visitor(ContentEraseVisitor(), request);
  for (auto itr(pending_requests_.begin()); itr != pending_requests_.end();) {
    if (itr->request.which() == request.which()) {
      message_id = boost::apply_visitor(MessageIdRequestVisitor(), itr->request);
      if ((message_id == request_message_id) && (source.group_id == itr->source.group_id))
        pending_requests_.erase(itr);
      else
        itr++;
    } else {
      itr++;
    }
  }
  boost::apply_visitor(ContentEraseVisitor(), request);
  handled_requests_.push_back(request);
  if (handled_requests_.size() > kMaxHandledRequestsCount_)
    handled_requests_.pop_front();
}

template<typename T>
std::vector<T> Accumulator<T>::Get(const T& request) {
  std::vector<T> requests;
  nfs::MessageId message_id;
  auto request_message_id(boost::apply_visitor(MessageIdRequestVisitor(), request));
  for (auto pending_request : pending_requests_) {
    if (pending_request.request.which() == request.which()) {
      message_id = boost::apply_visitor(MessageIdRequestVisitor(), pending_request.request);
      if ((message_id == request_message_id))
        requests.push_back(pending_request.request);
    }
  }
  return std::move(requests);
}

}  // namespace vault

}  // namespace maidsafe


#endif  // MAIDSAFE_VAULT_ACCUMULATOR_INL_H_
