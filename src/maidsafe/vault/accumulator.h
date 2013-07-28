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
#include <utility>
#include <vector>

#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/message.h"
#include "maidsafe/nfs/reply.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/nfs/utils.h"


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


template<typename Name>
class Accumulator {
 public:
  struct PendingRequest {
    PendingRequest(const nfs::Message& msg_in,
                   const routing::ReplyFunctor& reply_functor_in,
                   const nfs::Reply& reply_in);
    PendingRequest(const PendingRequest& other);
    PendingRequest& operator=(const PendingRequest& other);
    PendingRequest(PendingRequest&& other);
    PendingRequest& operator=(PendingRequest&& other);

    nfs::Message msg;
    routing::ReplyFunctor reply_functor;
    nfs::Reply reply;
  };

  struct HandledRequest {
    HandledRequest(const nfs::MessageId& msg_id_in,
                   const Name& account_name_in,
                   const nfs::MessageAction& action_type_in,
                   const Identity& data_name_in,
                   const DataTagValue& data_type_in,
                   const int32_t& size_in,
                   const nfs::Reply& reply_in);
    HandledRequest(const nfs::MessageId& msg_id_in,
                   const Name& account_name_in,
                   const nfs::Reply& reply_in);
    HandledRequest(const HandledRequest& other);
    HandledRequest& operator=(const HandledRequest& other);
    HandledRequest(HandledRequest&& other);
    HandledRequest& operator=(HandledRequest&& other);

    nfs::MessageId msg_id;
    Name account_name;
    nfs::MessageAction action;
    Identity data_name;
    DataTagValue data_type;
    int32_t size;
    nfs::Reply reply;
  };

  typedef TaggedValue<NonEmptyString, struct SerialisedRequestsTag> serialised_requests;

  Accumulator();

  // Returns true and populates <reply_out> if the message has already been set as handled.  If the
  // corresponding return_code != success, the message gets set in the reply.
  bool CheckHandled(const nfs::Message& message, nfs::Reply& reply_out) const;
  std::vector<nfs::Reply> GetPendingResults(const nfs::Message& message) const;
  // Adds a request with its individual result, pending the overall result of the operation.
  std::vector<nfs::Reply> PushSingleResult(const nfs::Message& message,
                                           const routing::ReplyFunctor& reply_functor,
                                           const nfs::Reply& reply);
  // Marks the message as handled and returns all pending requests held with the same ID
  std::vector<PendingRequest> SetHandled(const nfs::Message& message,
                                         const nfs::Reply& reply);
  // This will set handled and reply with success if a reply functor exists
  // safe to call many times
  void SetHandledAndReply(const nfs::MessageId message_id,
                          const NodeId& source_node,
                          const nfs::Reply& reply_out);
  friend class test::AccumulatorTest_BEH_PushSingleResult_Test;
  friend class test::AccumulatorTest_BEH_PushSingleResultThreaded_Test;
  friend class test::AccumulatorTest_BEH_CheckPendingRequestsLimit_Test;
  friend class test::AccumulatorTest_BEH_CheckHandled_Test;
  friend class test::AccumulatorTest_BEH_SetHandled_Test;
  friend class test::AccumulatorTest_BEH_FindHandled_Test;

 private:
  Accumulator(const Accumulator&);
  Accumulator& operator=(const Accumulator&);
  Accumulator(Accumulator&&);
  Accumulator& operator=(Accumulator&&);
  typename std::deque<HandledRequest>::const_iterator FindHandled(
      const nfs::Message& message) const;

  std::deque<PendingRequest> pending_requests_;
  std::deque<HandledRequest> handled_requests_;
  const size_t kMaxPendingRequestsCount_, kMaxHandledRequestsCount_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/accumulator-inl.h"

#endif  // MAIDSAFE_VAULT_ACCUMULATOR_H_
