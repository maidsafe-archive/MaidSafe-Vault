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

#ifndef MAIDSAFE_VAULT_ACCUMULATOR_H_
#define MAIDSAFE_VAULT_ACCUMULATOR_H_

#include <deque>

#include "maidsafe/nfs/message_types.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/nfs/vault/messages.h"


namespace maidsafe {

namespace vault {

namespace test {
class AccumulatorTest_BEH_PushSingleResult_Test;
class AccumulatorTest_BEH_PushSingleResultThreaded_Test;
class AccumulatorTest_BEH_CheckPendingRequestsLimit_Test;
class AccumulatorTest_BEH_CheckHandled_Test;
class AccumulatorTest_BEH_SetHandled_Test;
class AccumulatorTest_BEH_FindHandled_Test;
}  // namespace test

namespace {

template <typename RequestType>
class HasMessageId {
  typedef char Yes;
  typedef long No;

  template <typename C> static Yes Check(decltype(&C::message_id)) ;
  template <typename C> static No Check(...);

 public:
    static bool const value = sizeof(Check<RequestType>(0)) == sizeof(Yes);
};

class MessageIdRequestVisitor : public boost::static_visitor<nfs::MessageId> {
 public:
  template<typename T>
  nfs::MessageId operator()(const T& message) const {
    static_assert(HasMessageId<T>::value, "Input parameter must have message_id");
    return message.message_id;
  }
};

class ContentEraseVisitor : public boost::static_visitor<> {
 public:
  template<typename ContentType>
  void operator()(ContentType& content) {}
};

template<>
void ContentEraseVisitor::operator()(nfs_vault::DataNameAndContent& name_and_content) {
  name_and_content.content = NonEmptyString(std::string("NA"));
}

} // noname namespace

template<typename T>
class Accumulator {
 public:
  typedef T type;
  enum class AddResult { kSuccess, kWaiting, kFailure, kHandled };
  typedef std::function<AddResult(const std::vector<T>&)> AddCheckerFunctor;
  class AddRequestChecker {
    public:
     explicit AddRequestChecker(const size_t& required_requests)
         : required_requests_(required_requests) {}

     AddResult operator()(const std::vector<T>& requests) {
       if (requests.size() == required_requests_)
         return AddResult::kSuccess;
       else
         return AddResult::kWaiting;
     }

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

  AddResult AddPendingRequest(const T& request,
                              const routing::GroupSource& source,
                              AddCheckerFunctor checker);
  bool CheckHandled(const T& request);
  void SetHandled(const T& request, const routing::GroupSource& source);
  std::vector<T> Get(const T& request);

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

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/accumulator-inl.h"

#endif  // MAIDSAFE_VAULT_ACCUMULATOR_H_
