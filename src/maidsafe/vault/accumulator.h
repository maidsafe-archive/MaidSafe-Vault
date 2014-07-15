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

namespace maidsafe {

namespace vault {

namespace detail {

class MessageIdRequestVisitor : public boost::static_visitor<nfs::MessageId> {
 public:
  template <typename T>
  nfs::MessageId operator()(const T& message) const {
    //    static_assert(HasMessageId<T>::value, "Input parameter must have message_id");
    return message.id;
  }
};

/*
class ContentEraseVisitor : public boost::static_visitor<> {
 public:
  template <typename ContentType>
  void operator()(ContentType& content) {}
};

template <>
void ContentEraseVisitor::operator()(nfs_vault::DataNameAndContent& name_and_content) {
  name_and_content.content = NonEmptyString(std::string("NA"));
}
*/

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
  typedef std::function<AddResult(const std::vector<T>&)> AddCheckerFunctor;
  class AddRequestChecker {
   public:
    explicit AddRequestChecker(size_t required_requests) : required_requests_(required_requests) {
      assert((required_requests <= routing::Parameters::group_size) &&
             "Invalid number of requests");
    }

    AddResult operator()(const std::vector<T>& requests);

   private:
    size_t required_requests_;
  };

  struct PendingRequest {
    PendingRequest(const T& request_in, const routing::GroupSource& source_in)
        : request(request_in), source(source_in) {}
    T request;
    routing::GroupSource source;
  };

  Accumulator();

  AddResult AddPendingRequest(const T& request, const routing::GroupSource& source,
                              AddCheckerFunctor checker);
  AddResult AddPendingRequest(const T& request, const routing::SingleSource& source,
                              AddCheckerFunctor checker);

  bool CheckHandled(const T& request);
  //  void SetHandled(const T& request, const routing::GroupSource& source);
  std::vector<T> Get(const T& request, const routing::GroupSource& source);

 private:
  Accumulator(const Accumulator&);
  Accumulator& operator=(const Accumulator&);
  Accumulator(Accumulator&&);
  Accumulator& operator=(Accumulator&&);

  bool RequestExists(const T& request, const routing::GroupSource& source);

  std::deque<PendingRequest> pending_requests_;
  std::deque<T> handled_requests_;
  const size_t kMaxPendingRequestsCount_, kMaxHandledRequestsCount_;
};

template <typename T>
Accumulator<T>::Accumulator()
    : pending_requests_(),
      handled_requests_(),
      kMaxPendingRequestsCount_(300),
      kMaxHandledRequestsCount_(1000) {}

template <typename T>
typename Accumulator<T>::AddResult Accumulator<T>::AddPendingRequest(
    const T& request, const routing::GroupSource& source, AddCheckerFunctor checker) {
  LOG(kVerbose) << "Accumulator::AddPendingRequest for GroupSource "
                << HexSubstr(source.group_id.data.string()) << " sent from "
                << HexSubstr(source.sender_id->string());
  if (CheckHandled(request)) {
    LOG(kInfo) << "Accumulator::AddPendingRequest request has been handled";
    return Accumulator<T>::AddResult::kHandled;
  }

  if (!RequestExists(request, source)) {
    pending_requests_.push_back(PendingRequest(request, source));
    LOG(kVerbose) << "Accumulator::AddPendingRequest has " << pending_requests_.size()
                  << " pending requests, allowing " << kMaxPendingRequestsCount_ << " requests";
    if (pending_requests_.size() > kMaxPendingRequestsCount_)
      pending_requests_.pop_front();
  } else {
    LOG(kInfo) << "Accumulator::AddPendingRequest request already existed";
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
bool Accumulator<T>::CheckHandled(const T& request) {
  auto request_message_id(boost::apply_visitor(detail::MessageIdRequestVisitor(), request));
  nfs::MessageId message_id;
  for (auto handled_request : handled_requests_) {
    if (handled_request.which() == request.which()) {
      message_id = boost::apply_visitor(detail::MessageIdRequestVisitor(), handled_request);
      if (message_id == request_message_id)
        return true;
    }
  }
  return false;
}

// template<typename T>
// void Accumulator<T>::SetHandled(const T& request, const routing::GroupSource& source) {
//    assert(!CheckHandled(request) && "Request has already been set as handled");
//    nfs::MessageId message_id;
//    auto request_message_id(boost::apply_visitor(detail::MessageIdRequestVisitor(), request));
//    boost::apply_visitor(detail::ContentEraseVisitor(), request);
//    for (auto itr(std::begin(pending_requests_)); itr != std::end(pending_requests_);) {
//        if (itr->request.which() == request.which()) {
//            message_id = boost::apply_visitor(detail::MessageIdRequestVisitor(), itr->request);
//            if ((message_id == request_message_id) && (source.group_id == itr->source.group_id))
//                pending_requests_.erase(itr);
//            else
//                ++itr;
//        } else {
//            ++itr;
//        }
//    }
//    boost::apply_visitor(detail::ContentEraseVisitor(), request);
//    handled_requests_.push_back(request);
//    if (handled_requests_.size() > kMaxHandledRequestsCount_)
//        handled_requests_.pop_front();
// }

template <typename T>
std::vector<T> Accumulator<T>::Get(const T& request, const routing::GroupSource& source) {
  std::vector<T> requests;
  nfs::MessageId message_id;
  auto request_message_id(boost::apply_visitor(detail::MessageIdRequestVisitor(), request));
  for (auto pending_request : pending_requests_) {
    if (pending_request.request.which() == request.which()) {
      message_id = boost::apply_visitor(detail::MessageIdRequestVisitor(), pending_request.request);
      if (((message_id == request_message_id)) &&
          (source.group_id == pending_request.source.group_id))
        requests.push_back(pending_request.request);
    }
  }
  LOG(kVerbose) << requests.size() << " requests are found for the request bearing message id "
                << request_message_id.data;
  return requests;
}

template <typename T>
bool Accumulator<T>::RequestExists(const T& request, const routing::GroupSource& source) {
  auto request_message_id(boost::apply_visitor(detail::MessageIdRequestVisitor(), request));
  for (auto pending_request : pending_requests_)
    if (request_message_id ==
            boost::apply_visitor(detail::MessageIdRequestVisitor(), pending_request.request) &&
        source == pending_request.source && pending_request.request == request) {
      LOG(kWarning) << "Accumulator<T>::RequestExists,  reguest with message id "
                    << request_message_id.data << " from sender "
                    << HexSubstr(source.sender_id->string()) << " with group_id "
                    << HexSubstr(source.group_id->string())
                    << " already exists in the pending requests list";
      return true;
    }
  LOG(kInfo) << "Accumulator<T>::RequestExists,  reguest with message id "
             << request_message_id.data << " from sender " << HexSubstr(source.sender_id->string())
             << " with group_id " << HexSubstr(source.group_id->string())
             << " doesn't exists in the pending requests list";
  return false;
}

template <typename T>
typename Accumulator<T>::AddResult Accumulator<T>::AddRequestChecker::operator()(
    const std::vector<T>& requests) {
  LOG(kVerbose) << "Accumulator<T>::AddRequestChecker operator(),  required_requests_ : "
                << required_requests_ << " , checking against " << requests.size() << " requests";
  // BEFORE_RELEASE the following commented out code shall be reviewed
  //   if (requests.size() > routing::Parameters::group_size) {
  //     LOG(kError) << "Invalid number of requests, already have " << requests.size()
  //                 << " requests with a group size of " << routing::Parameters::group_size;
  //     return AddResult::kFailure;
  //   }
  if (requests.size() < required_requests_) {
    LOG(kInfo) << "Accumulator<T>::AddRequestChecke::operator() not enough pending requests";
    return AddResult::kWaiting;
  } else {
    auto index(0);
    while ((requests.size() - index) >= required_requests_) {
      if (std::count_if(std::begin(requests), std::end(requests), [&](const T& request) {
            if (requests.at(index) == request) {
              LOG(kVerbose) << "requests match each other";
              return true;
            } else {
              LOG(kVerbose) << "requests don't match each other";
              return false;
            }
          }) == static_cast<int>(required_requests_))
        return AddResult::kSuccess;
      ++index;
    }
  }
  LOG(kInfo) << "Accumulator<T>::AddRequestChecke::operator() the reqeust is still waiting";
  return AddResult::kWaiting;
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_ACCUMULATOR_H_
