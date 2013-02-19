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

#include "maidsafe/common/log.h"
#include "maidsafe/common/test.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/data_types/data_type_values.h"
#include "maidsafe/nfs/data_message.h"
#include "maidsafe/nfs/data_message_pb.h"
#include "maidsafe/nfs/types.h"


namespace maidsafe {

namespace vault {

namespace test {

namespace {

DataMessage::Action GenerateAction() {
  return static_cast<DataMessage::Action>(RandomUint32() % 3);
}

PersonaId GenerateSource() {
  PersonaId source;
  // matches Persona enum in types.h
  source.persona = static_cast<Persona>(RandomUint32() % 7);
  source.node_id = NodeId(NodeId::kRandomId);
  return source;
}
DataMessage MakeMessage() {
  DataMessage::Data data(static_cast<DataTagValue>(RandomUint32() % 13),
                         Identity(RandomString(NodeId::kSize)),
                         NonEmptyString(RandomString(1 + RandomUint32() % 50)),
                         GenerateAction());
  DataMessage data_message(
      static_cast<Persona>(RandomUint32() % 7),
      GenerateSource(),
      data,
      passport::PublicPmid::name_type(Identity(RandomString(NodeId::kSize))));
  return data_message;
}
}  // unnamed namespace

TEST(AccumulatorTest, BEH_PushRequest) {
  DataMessage data_message = MakeMessage();
  Reply reply(CommonErrors::success);
  Accumulator<passport::PublicMaid::name_type> accumulator;
  Accumulator<passport::PublicMaid::name_type>::Request request(data_message,
                                                                [](const std::string&) {},
                                                                reply);
  accumulator.PushRequest(request);
  EXPECT_EQ(accumulator.pending_requests_.size(), 1);
  auto request_identity(accumulator.pending_requests_.at(0).first);
  EXPECT_FALSE(accumulator.CheckHandled(request_identity, reply));
  accumulator.SetHandled(request_identity, reply);
  EXPECT_EQ(accumulator.pending_requests_.size(), 0);
  EXPECT_TRUE(accumulator.CheckHandled(request_identity, reply));
  Accumulator<passport::PublicMaid::name_type>::serialised_requests serialised(
      accumulator.SerialiseHandledRequests(request_identity.second));
  auto parsed(accumulator.ParseHandledRequests(serialised));
  EXPECT_EQ(parsed.size(), 1);
}

TEST(AccumulatorTest, BEH_PushRequestThreaded) {
  maidsafe::test::RunInParallel(10, [] {
    DataMessage data_message = MakeMessage();
    Reply reply(CommonErrors::success);
    Accumulator<passport::PublicMaid::name_type> accumulator;
    Accumulator<passport::PublicMaid::name_type>::Request request(data_message,
                                                                  [](const std::string&) {},
                                                                  reply);
    accumulator.PushRequest(request);
    EXPECT_EQ(accumulator.pending_requests_.size(), 1);
    auto request_identity(accumulator.pending_requests_.at(0).first);
    EXPECT_FALSE(accumulator.CheckHandled(request_identity, reply));
    accumulator.SetHandled(request_identity, reply);
    EXPECT_EQ(accumulator.pending_requests_.size(), 0);
    EXPECT_TRUE(accumulator.CheckHandled(request_identity, reply));
    Accumulator<passport::PublicMaid::name_type>::serialised_requests serialised(
        accumulator.SerialiseHandledRequests(request_identity.second));
    auto parsed(accumulator.ParseHandledRequests(serialised));
    EXPECT_EQ(parsed.size(), 1);
    });
}

TEST(AccumulatorTest, BEH_CheckPendingRequestsLimit) {
  Accumulator<passport::PublicPmid::name_type> accumulator;
  //  Pending list limit 300
  size_t pending_request_max_limit = accumulator.kMaxPendingRequestsCount_;
  for (size_t index = 0; index < pending_request_max_limit; ++index) {
    DataMessage data_message = MakeMessage();
    Reply reply(CommonErrors::success);
    Accumulator<passport::PublicPmid::name_type>::Request request(data_message,
                                                                  [](const std::string&) {},
                                                                  reply);
    accumulator.PushRequest(request);
    EXPECT_EQ(accumulator.pending_requests_.size(), (index + 1));
  }
  // Try to add request beyond the limit and it should fail
  EXPECT_EQ(accumulator.pending_requests_.size(), pending_request_max_limit);

  DataMessage data_message = MakeMessage();
  Reply reply(CommonErrors::unknown);
  Accumulator<passport::PublicPmid::name_type>::Request request(data_message,
                                                                [](const std::string&) {},
                                                                reply);
  accumulator.PushRequest(request);
  EXPECT_EQ(accumulator.pending_requests_.size(), pending_request_max_limit);
}

TEST(AccumulatorTest, BEH_CheckHandled) {
  DataMessage data_message = MakeMessage();
  Reply reply(CommonErrors::success);
  Accumulator<passport::PublicMaid::name_type> accumulator;
  Accumulator<passport::PublicMaid::name_type>::Request request(data_message,
                                                                [](const std::string&) {},
                                                                reply);
  auto request_identity(std::make_pair(MessageId(Identity(request.msg.message_id())),
                                       passport::PublicMaid::name_type(
                                           Identity(request.msg.source().node_id.string()))));
  EXPECT_FALSE(accumulator.CheckHandled(request_identity, reply));
  accumulator.PushRequest(request);
  accumulator.SetHandled(request_identity, reply);
  EXPECT_TRUE(accumulator.CheckHandled(request_identity, reply));
}

TEST(AccumulatorTest, BEH_SetHandled) {
  DataMessage data_message = MakeMessage();
  Reply reply(CommonErrors::success);
  Accumulator<passport::PublicPmid::name_type> accumulator;
  Accumulator<passport::PublicPmid::name_type>::Request request(data_message,
                                                                [](const std::string&) {},
                                                                reply);
 auto request_identity(std::make_pair(MessageId(Identity(request.msg.message_id())),
                                      passport::PublicPmid::name_type(Identity(request.msg.source().node_id.string()))));
 EXPECT_TRUE(accumulator.handled_requests_.empty());
 accumulator.SetHandled(request_identity, reply);
 EXPECT_TRUE(accumulator.handled_requests_.empty());
 accumulator.PushRequest(request);
 EXPECT_EQ(accumulator.pending_requests_.size(), 1);
 accumulator.SetHandled(request_identity, reply);
 EXPECT_EQ(accumulator.handled_requests_.size(), 1);
 EXPECT_TRUE(accumulator.pending_requests_.empty());
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
