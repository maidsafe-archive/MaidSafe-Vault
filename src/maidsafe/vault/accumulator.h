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
    // static_assert(HasMessageId<T>::value, "Input parameter must have message_id");
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
  typedef std::function<AddResult(const size_t&)> AddCheckerFunctor;
  class AddRequestChecker {
   public:
    explicit AddRequestChecker(size_t required_requests) : required_requests_(required_requests) {
      assert((required_requests <= routing::Parameters::group_size) &&
             "Invalid number of requests");
    }

    AddResult operator()(const size_t& number_of_requests);

   private:
    size_t required_requests_;
  };

  class PendingRequest {
   public:
    PendingRequest(const T& request, const routing::GroupSource& source)
        : request_(request), sources_(), time_(std::chrono::system_clock::now()) {
      sources_.push_back(source);
    }

    bool HasSource(const routing::GroupSource& source) const {
      auto result(std::find(std::begin(sources_), std::end(sources_), source));
      return (result != sources_.end());
    }

    void AddSource(const routing::GroupSource& source) {
      sources_.push_back(source);
    }

    size_t NumberOfRequests() const { return sources_.size(); }

    bool SameGroupId(const routing::GroupSource& source) const {
      if (sources_.empty())
        return false;
      return (sources_.begin()->group_id == source.group_id);
    }

    const T& Request() const { return request_; }
    bool HasExpired() const {
      std::chrono::seconds lifetime(
        std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::system_clock::now() - time_).count());
      return detail::Parameters::kDefaultLifetime < lifetime;
    }

   private:
    T request_;
    std::vector<routing::GroupSource> sources_;
    std::chrono::steady_clock::time_point time_;
  };

  explicit Accumulator();

  AddResult AddPendingRequest(const T& request, const routing::GroupSource& source,
                              AddCheckerFunctor checker);
  AddResult AddPendingRequest(const T& request, const routing::SingleSource& source,
                              AddCheckerFunctor checker);
  AddResult AddPendingRequest(const T& request, const routing::SingleRelaySource& source,
                              AddCheckerFunctor checker);

  size_t Get(const T& request, const routing::GroupSource& source);

 private:
  Accumulator(const Accumulator&);
  Accumulator& operator=(const Accumulator&);
  Accumulator(Accumulator&&);
  Accumulator& operator=(Accumulator&&);

  bool AddRequest(const T& request, const routing::GroupSource& source);

  std::deque<PendingRequest> pending_requests_;
};

template <typename T>
Accumulator<T>::Accumulator() : pending_requests_() {}

template <typename T>
typename Accumulator<T>::AddResult Accumulator<T>::AddPendingRequest(
    const T& request, const routing::GroupSource& source, AddCheckerFunctor checker) {
  LOG(kVerbose) << "Accumulator::AddPendingRequest for GroupSource "
                << HexSubstr(source.group_id.data.string()) << " sent from "
                << HexSubstr(source.sender_id->string());
  if (!AddRequest(request, source)) {
    LOG(kInfo) << "Accumulator::AddPendingRequest request already exists";
    return AddResult::kWaiting;
  }
  return checker(Get(request, source));
}

template <typename T>
typename Accumulator<T>::AddResult Accumulator<T>::AddPendingRequest(
    const T& /*request*/, const routing::SingleSource& /*source*/, AddCheckerFunctor /*checker*/) {
  LOG(kVerbose) << "Accumulator::AddPendingRequest for SingleSource -- Always return success";
  return AddResult::kSuccess;
}

template <typename T>
typename Accumulator<T>::AddResult Accumulator<T>::AddPendingRequest(
    const T& /*request*/, const routing::SingleRelaySource& /*source*/,
    AddCheckerFunctor /*checker*/) {
  LOG(kVerbose) << "Accumulator::AddPendingRequest for SingleSource -- Always return success";
  return AddResult::kSuccess;
}

template <typename T>
size_t Accumulator<T>::Get(const T& request, const routing::GroupSource& source) {
  size_t number_of_requests(0);
  bool request_found(false);
  auto it(pending_requests_.begin());

  while (it != pending_requests_.end()) {
    if (it->HasExpired()) {
      it = pending_requests_.erase(it);
      continue;
    }
    if (!request_found) {
      if (it->Request() == request && it->SameGroupId(source)) {
        number_of_requests = it->NumberOfRequests();
        request_found = true;
      }
    }
    ++it;
  }

  LOG(kVerbose) << number_of_requests << " requests were found for the request with message id "
                << boost::apply_visitor(detail::MessageIdRequestVisitor(), request).data;
  return number_of_requests;
}

template <typename T>
bool Accumulator<T>::AddRequest(const T& request, const routing::GroupSource& source) {
  for (auto& pending_request : pending_requests_) {
    bool identical_requests(pending_request.Request() == request);
    if (pending_request.HasSource(source) && identical_requests) {
      LOG(kWarning) << "Accumulator<T>::AddRequest, request with message id "
                    << boost::apply_visitor(detail::MessageIdRequestVisitor(), request).data
                    << " from sender " << HexSubstr(source.sender_id->string())
                    << " with group_id " << HexSubstr(source.group_id->string())
                    << " already exists in the pending requests list";
      return false;
    } else if (identical_requests) {
      if (!pending_request.SameGroupId(source))
        break;

      pending_request.AddSource(source);
      LOG(kInfo) << "Accumulator<T>::AddRequest, request with message id "
                 << boost::apply_visitor(detail::MessageIdRequestVisitor(), request).data
                 << " from sender " << HexSubstr(source.sender_id->string()) << " with group_id "
                 << HexSubstr(source.group_id->string())
                 << " has source added to an existing pending request.";
      return true;
    }
  }

  pending_requests_.push_back(PendingRequest(request, source));
  LOG(kInfo) << "Accumulator<T>::AddRequest, request with message id "
             << boost::apply_visitor(detail::MessageIdRequestVisitor(), request).data
             << " from sender " << HexSubstr(source.sender_id->string()) << " with group_id "
             << HexSubstr(source.group_id->string())
             << " has been added to the pending requests list";
  return true;
}

template <typename T>
typename Accumulator<T>::AddResult Accumulator<T>::AddRequestChecker::operator()(
    const size_t& number_of_requests) {
  LOG(kVerbose) << "Accumulator<T>::AddRequestChecker operator(), required requests : "
                << required_requests_ << " , checking against " << number_of_requests
                << " actual requests";

  if (number_of_requests < required_requests_) {
    LOG(kInfo) << "Accumulator<T>::AddRequestChecker::operator() request still pending";
    return AddResult::kWaiting;
  } else if (number_of_requests > required_requests_) {
    LOG(kInfo) << "Accumulator<T>::AddRequestChecker::operator() request already handled";
    return AddResult::kHandled;
  } else {
    LOG(kInfo) << "Accumulator<T>::AddRequestChecker::operator() request successful";
    return AddResult::kSuccess;
  }
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_ACCUMULATOR_H_
