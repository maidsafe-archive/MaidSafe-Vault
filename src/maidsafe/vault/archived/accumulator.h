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

#ifndef MAIDSAFE_VAULT_ACCUMULATOR_H_
#define MAIDSAFE_VAULT_ACCUMULATOR_H_

#include <algorithm>
#include <deque>
#include <string>
#include <vector>

#include "maidsafe/common/data_types/data_name_variant.h"
#include "maidsafe/nfs/utils.h"
#include "maidsafe/nfs/message_types.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/nfs/vault/messages.h"
#include "maidsafe/vault/handled_request.pb.h"
#include "maidsafe/vault/types.h"
#include "maidsafe/vault/parameters.h"

namespace maidsafe {

namespace vault {

namespace detail {

class MessageIdRequestVisitor : public boost::static_visitor<nfs::MessageId> {
 public:
  template <typename T>
  nfs::MessageId operator()(const T& message) const {
    return message.id;
  }
};

}  // namespace detail

template <typename T>
class Accumulator {
 public:
  typedef T type;
  enum class AddResult {
    kSuccess,
    kWaiting,
    kFailure,
    kHandled
  };
  typedef std::function<AddResult(size_t)> AddCheckerFunctor;

  class AddRequestChecker {
   public:
    explicit AddRequestChecker(size_t required_requests);

    AddResult operator()(size_t number_of_requests);

   private:
    size_t required_requests_;
  };

  class PendingRequest {
   public:
    PendingRequest(const T& request, const routing::GroupSource& source,
                   const std::chrono::steady_clock::duration& time_to_live);

    void AddSource(const routing::GroupSource& source);
    bool HasSource(const routing::GroupSource& source) const;
    bool SameGroupId(const routing::GroupSource& source) const;

    const T& Request() const;
    size_t NumberOfRequests() const;

    bool HasExpired() const;

   private:
    T request_;
    std::vector<routing::GroupSource> sources_;
    std::chrono::steady_clock::time_point time_;
    std::chrono::steady_clock::duration time_to_live_;
  };

  explicit Accumulator(
      const std::chrono::steady_clock::duration& time_to_live = std::chrono::seconds(300));

  AddResult AddPendingRequest(const T& request, const routing::GroupSource& source,
                              AddCheckerFunctor checker);
  AddResult AddPendingRequest(const T& request, const routing::SingleSource& source,
                              AddCheckerFunctor checker);
  AddResult AddPendingRequest(const T& request, const routing::SingleRelaySource& source,
                              AddCheckerFunctor checker);

 private:
  Accumulator(const Accumulator&);
  Accumulator& operator=(const Accumulator&);
  Accumulator(Accumulator&&);
  Accumulator& operator=(Accumulator&&);

  size_t AddRequest(const T& request, const routing::GroupSource& source);

  std::deque<PendingRequest> pending_requests_;
  std::chrono::steady_clock::duration time_to_live_;
};

template <typename T>
Accumulator<T>::Accumulator(const std::chrono::steady_clock::duration& time_to_live)
  : pending_requests_(), time_to_live_(time_to_live) {}

template <typename T>
typename Accumulator<T>::AddResult Accumulator<T>::AddPendingRequest(
    const T& request, const routing::GroupSource& source, AddCheckerFunctor checker) {
  return checker(AddRequest(request, source));
}

template <typename T>
typename Accumulator<T>::AddResult Accumulator<T>::AddPendingRequest(
    const T& /*request*/, const routing::SingleSource& /*source*/, AddCheckerFunctor /*checker*/) {
  return AddResult::kSuccess;
}

template <typename T>
typename Accumulator<T>::AddResult Accumulator<T>::AddPendingRequest(
    const T& /*request*/, const routing::SingleRelaySource& /*source*/,
    AddCheckerFunctor /*checker*/) {
  return AddResult::kSuccess;
}

template <typename T>
size_t Accumulator<T>::AddRequest(const T& request, const routing::GroupSource& source) {
  size_t number_of_requests(0);
  bool request_found(false);
  auto it(pending_requests_.begin());

  while (it != pending_requests_.end()) {
    if (!request_found) {
      bool identical_requests(it->Request() == request);
      if (identical_requests && it->HasSource(source)) {
        number_of_requests = it->NumberOfRequests();
        request_found = true;
      } else if (identical_requests) {
        if (it->SameGroupId(source)) {
          it->AddSource(source);
          number_of_requests = it->NumberOfRequests();
          request_found = true;
        }
      }
    }
    if (it->HasExpired())
      it = pending_requests_.erase(it);
    else
      ++it;
  }

  if (!request_found) {
    pending_requests_.push_back(PendingRequest(request, source, time_to_live_));
    number_of_requests = 1;
  }
  return number_of_requests;
}

template <typename T>
Accumulator<T>::AddRequestChecker::AddRequestChecker(size_t required_requests)
    : required_requests_(required_requests) {
  assert((required_requests <= routing::Parameters::group_size) && "Invalid number of requests");
}

template <typename T>
typename Accumulator<T>::AddResult Accumulator<T>::AddRequestChecker::operator()(
    size_t number_of_requests) {
  if (number_of_requests < required_requests_)
    return AddResult::kWaiting;
  else if (number_of_requests > required_requests_)
    return AddResult::kHandled;
  else
    return AddResult::kSuccess;
}

template <typename T>
Accumulator<T>::PendingRequest::PendingRequest(const T& request,
  const routing::GroupSource& source, const std::chrono::steady_clock::duration& time_to_live)
    : request_(request), sources_(), time_(std::chrono::steady_clock::now()),
      time_to_live_(time_to_live) {
  sources_.push_back(source);
}

template <typename T>
void Accumulator<T>::PendingRequest::AddSource(const routing::GroupSource& source) {
  sources_.push_back(source);
}

template <typename T>
bool Accumulator<T>::PendingRequest::HasSource(const routing::GroupSource& source) const {
  return (std::find(sources_.begin(), sources_.end(), source) != sources_.end());
}

template <typename T>
bool Accumulator<T>::PendingRequest::SameGroupId(const routing::GroupSource& source) const {
  return (sources_.begin()->group_id == source.group_id);
}

template <typename T>
const T& Accumulator<T>::PendingRequest::Request() const { return request_; }

template <typename T>
size_t Accumulator<T>::PendingRequest::NumberOfRequests() const { return sources_.size(); }

template <typename T>
bool Accumulator<T>::PendingRequest::HasExpired() const {
  std::chrono::steady_clock::time_point now(std::chrono::steady_clock::now());
  std::chrono::seconds
      lifetime(std::chrono::duration_cast<std::chrono::seconds>(now - time_).count());
  return time_to_live_ <= lifetime;
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_ACCUMULATOR_H_
