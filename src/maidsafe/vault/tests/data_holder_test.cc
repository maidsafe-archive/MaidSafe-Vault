 /*******************************************************************************
 *  Copyright 2012 maidsafe.net limited                                        *
 *                                                                             *
 *  The following source code is property of maidsafe.net limited and is not   *
 *  meant for external use.  The use of this code is governed by the licence   *
 *  file licence.txt found in the root of this directory and also on           *
 *  www.maidsafe.net.                                                          *
 *                                                                             *
 *  You are not free to copy, amend or otherwise use this source code without  *
 *  the explicit written permission of the board of directors of maidsafe.net. *
 ******************************************************************************/

#include <memory>
#include "boost/filesystem/path.hpp"

#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/common/test.h"

#include "maidsafe/nfs/message.h"
#include "maidsafe/vault/data_holder.h"

namespace maidsafe {

namespace vault {

namespace test {
namespace {
//maidsafe::test::TestPath test_path(
//                      maidsafe::test::CreateTestPath("MaidSafe_Test_Vault"));
  boost::filesystem::path test_path("/tmp/MaidSafe_Test_Vault");
}

class DataHolderTest : public testing::Test {
 public:
  DataHolderTest()
    : vault_root_directory_(test_path),
        data_holder_(vault_root_directory_) {
  }
 protected:
  template <typename Data>
  void HandlePutMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor) {
    data_holder_.HandlePutMessage<Data>(message, reply_functor);
  }

  template <typename Data>
  void HandleGetMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor) {
    data_holder_.HandleGetMessage<Data>(message, reply_functor);
  }

  template <typename Data>
  void HandleDeleteMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor) {
    data_holder_.HandleDeleteMessage<Data>(message, reply_functor);
  }

  template <typename Data>
  bool IsInCach(const nfs::Message& message) {
    return data_holder_.IsInCache<Data>(message);
  }

  template <typename Data>
  void StoreInCach(const nfs::Message& message) {
    data_holder_.StoreInCache<Data>(message);
  }

  boost::filesystem::path vault_root_directory_;
  DataHolder data_holder_;
};

TEST_F(DataHolderTest, BEH_StoreInCache) {
  nfs::Message::Destination destination(nfs::Message::Peer(nfs::PersonaType::kDataHolder,
                                                           NodeId(NodeId::kRandomId)));
  nfs::Message::Source source(nfs::Message::Peer(nfs::PersonaType::kPmidAccountHolder,
                                                 NodeId(NodeId::kRandomId)));
  NonEmptyString content(RandomAlphaNumericString(256));
  asymm::Signature signature;
  nfs::Message message(nfs::ActionType::kPut, destination, source,
                       detail::DataTagValue::kAnmaidValue, content, signature);
  EXPECT_FALSE(this->IsInCach<passport::Anmaid>(message));
  this->StoreInCach<passport::Anmaid>(message);
  EXPECT_TRUE(this->IsInCach<passport::Anmaid>(message));
}

TEST_F(DataHolderTest, BEH_HandlePutMessage) {
  nfs::Message::Destination destination(nfs::Message::Peer(nfs::PersonaType::kDataHolder,
                                                           NodeId(NodeId::kRandomId)));
  nfs::Message::Source source(nfs::Message::Peer(nfs::PersonaType::kPmidAccountHolder,
                                                 NodeId(NodeId::kRandomId)));
  NonEmptyString content(RandomAlphaNumericString(256));
  asymm::Signature signature;
  nfs::Message message(nfs::ActionType::kPut, destination, source,
                       detail::DataTagValue::kAnmaidValue, content, signature);

  std::string retrieved;
  this->HandlePutMessage<passport::Anmaid>(message, [&](const std::string&) {});
  this->HandleGetMessage<passport::Anmaid>(message,
                                           [&](const std::string& data) {
                                             retrieved = data;
                                           });
  EXPECT_NE(retrieved.find(content.string()), -1);
}

TEST_F(DataHolderTest, BEH_HandleGetMessage) {
  nfs::Message::Destination destination(nfs::Message::Peer(nfs::PersonaType::kDataHolder,
                                                           NodeId(NodeId::kRandomId)));
  nfs::Message::Source source(nfs::Message::Peer(nfs::PersonaType::kPmidAccountHolder,
                                                 NodeId(NodeId::kRandomId)));
  const NonEmptyString content(RandomAlphaNumericString(256));
  const asymm::Signature signature;
  nfs::Message message(nfs::ActionType::kGet, destination, source,
                       detail::DataTagValue::kAnmaidValue, content, signature);
  std::string retrieved;
  this->HandleGetMessage<passport::Anmaid>(message,
                                           [&](const std::string& data) {
                                             retrieved = data;
                                           });
  EXPECT_EQ(retrieved, nfs::ReturnCode(-1).Serialise()->string());
}

//TEST(DataHolderTest, BEH_HandlePostMessage) {
//}

TEST_F(DataHolderTest, BEH_HandleDeleteMessage) {
  nfs::Message::Destination destination(nfs::Message::Peer(nfs::PersonaType::kDataHolder,
                                                           NodeId(NodeId::kRandomId)));
  nfs::Message::Source source(nfs::Message::Peer(nfs::PersonaType::kPmidAccountHolder,
                                                 NodeId(NodeId::kRandomId)));
  NonEmptyString content(RandomAlphaNumericString(256));
  asymm::Signature signature;
  nfs::Message message(nfs::ActionType::kPut, destination, source,
                       detail::DataTagValue::kAnmaidValue, content, signature);

  std::string retrieved;
  this->HandlePutMessage<passport::Anmaid>(message, [&](const std::string&) {});
  this->HandleGetMessage<passport::Anmaid>(message,
                                           [&](const std::string& data) {
                                             retrieved = data;
                                           });
  EXPECT_NE(retrieved.find(content.string()), -1);
  this->HandleDeleteMessage<passport::Anmaid>(message,
                                              [&](const std::string& data) {
                                                retrieved = data;
                                              });
  EXPECT_EQ(retrieved, nfs::ReturnCode(0).Serialise()->string());
  this->HandleGetMessage<passport::Anmaid>(message,
                                           [&](const std::string& data) {
                                             retrieved = data;
                                           });
  EXPECT_EQ(retrieved, nfs::ReturnCode(-1).Serialise()->string());
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe

