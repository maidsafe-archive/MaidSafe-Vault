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
                   const maidsafe_error& return_code_in);
    PendingRequest(const PendingRequest& other);
    PendingRequest& operator=(const PendingRequest& other);
    PendingRequest(PendingRequest&& other);
    PendingRequest& operator=(PendingRequest&& other);

    nfs::Message msg;
    routing::ReplyFunctor reply_functor;
    maidsafe_error return_code;
  };

  struct HandledRequest {
    HandledRequest(const nfs::MessageId& msg_id_in,
                   const Name& account_name_in,
                   const nfs::MessageAction& action_type_in,
                   const Identity& data_name,
                   const DataTagValue& data_type,
                   const int32_t& size_in,
                   const maidsafe_error& return_code_in);
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
    maidsafe_error return_code;
  };

  typedef TaggedValue<NonEmptyString, struct SerialisedRequestsTag> serialised_requests;

  Accumulator();

  // Returns true and populates <reply_out> if the message has already been set as handled.  If the
  // corresponding return_code != success, the message gets set in the reply.
  bool CheckHandled(const nfs::Message& message, nfs::Reply& reply_out) const;
  // Adds a request with its individual result, pending the overall result of the operation.
  std::vector<nfs::Reply> PushSingleResult(const nfs::Message& message,
                                           const routing::ReplyFunctor& reply_functor,
                                           const maidsafe_error& return_code);
  // Marks the message as handled and returns all pending requests held with the same ID
  std::vector<PendingRequest> SetHandled(const nfs::Message& message,
                                         const maidsafe_error& return_code);
  // Returns all handled requests for the given account name.
  std::vector<HandledRequest> Get(const Name& name) const;
  // Serialises all handled requests for the given account name.
  serialised_requests Serialise(const Name& name) const;
  // Parses the list of serialised handled requests.
  std::vector<HandledRequest> Parse(const serialised_requests& serialised_requests_in) const;

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
