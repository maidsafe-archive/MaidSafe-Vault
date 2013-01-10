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

#include "maidsafe/vault/data_holder.h"

#include <memory>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/common/test.h"

#include "maidsafe/nfs/message.h"


namespace maidsafe {

namespace vault {

namespace test {

template<class T>
class DataHolderTest : public testing::Test {
 public:
  DataHolderTest()
      : vault_root_directory_(maidsafe::test::CreateTestPath("MaidSafe_Test_DataHolder")),
        data_holder_(*vault_root_directory_) {}

 protected:
  void HandlePutMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor) {
    data_holder_.HandlePutMessage<T>(message, reply_functor);
  }
  void HandleGetMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor) {
    data_holder_.HandleGetMessage<T>(message, reply_functor);
  }
  void HandleDeleteMessage(const nfs::Message& message,
                           const routing::ReplyFunctor& reply_functor) {
    data_holder_.HandleDeleteMessage<T>(message, reply_functor);
  }

  maidsafe::test::TestPath vault_root_directory_;
  DataHolder data_holder_;
};

TYPED_TEST_CASE_P(DataHolderTest);

TYPED_TEST_P(DataHolderTest, BEH_HandlePutMessage) {
  nfs::Message::Source source(nfs::PersonaType::kPmidAccountHolder, NodeId(NodeId::kRandomId));
  NonEmptyString content(RandomAlphaNumericString(256));
  asymm::Signature signature;
  nfs::Message message(nfs::ActionType::kPut, nfs::PersonaType::kDataHolder, source,
                       detail::DataTagValue::kAnmaidValue, Identity(RandomString(NodeId::kSize)),
                       content, signature);

  std::string retrieved;
  this->HandlePutMessage(message, [&](const std::string&) {});
  this->HandleGetMessage(message,
                         [&](const std::string& data) {
                           retrieved = data;
                         });
  EXPECT_NE(retrieved.find(content.string()), -1);
}

TYPED_TEST_P(DataHolderTest, BEH_HandleGetMessage) {
  nfs::Message::Source source(nfs::PersonaType::kPmidAccountHolder, NodeId(NodeId::kRandomId));
  const NonEmptyString content(RandomAlphaNumericString(256));
  const asymm::Signature signature;
  nfs::Message message(nfs::ActionType::kGet, nfs::PersonaType::kDataHolder, source,
                       detail::DataTagValue::kAnmaidValue, Identity(RandomString(NodeId::kSize)),
                       content, signature);
  std::string retrieved;
  this->HandleGetMessage(message,
                         [&](const std::string& data) {
                           retrieved = data;
                          });
  EXPECT_EQ(retrieved, nfs::ReturnCode(-1).Serialise()->string());
}

TYPED_TEST_P(DataHolderTest, BEH_HandleDeleteMessage) {
  nfs::Message::Source source(nfs::PersonaType::kPmidAccountHolder, NodeId(NodeId::kRandomId));
  NonEmptyString content(RandomAlphaNumericString(256));
  asymm::Signature signature;
  nfs::Message message(nfs::ActionType::kPut, nfs::PersonaType::kDataHolder, source,
                       detail::DataTagValue::kAnmaidValue, Identity(RandomString(NodeId::kSize)),
                       content, signature);

  std::string retrieved;
  this->HandlePutMessage(message, [&](const std::string&) {});
  this->HandleGetMessage(message,
                         [&](const std::string& data) {
                           retrieved = data;
                          });
  EXPECT_NE(retrieved.find(content.string()), -1);
  this->HandleDeleteMessage(message,
                            [&](const std::string& data) {
                              retrieved = data;
                            });
  EXPECT_EQ(retrieved, nfs::ReturnCode(0).Serialise()->string());
  this->HandleGetMessage(message,
                         [&](const std::string& data) {
                           retrieved = data;
                         });
  EXPECT_EQ(retrieved, nfs::ReturnCode(-1).Serialise()->string());
}

REGISTER_TYPED_TEST_CASE_P(DataHolderTest,
                           BEH_HandlePutMessage,
                           BEH_HandleGetMessage,
                           BEH_HandleDeleteMessage);

typedef testing::Types<passport::PublicAnmid,
                       passport::PublicAnsmid,
                       passport::PublicAntmid,
                       passport::PublicAnmaid,
                       passport::PublicMaid,
                       passport::PublicPmid,
                       passport::Mid,
                       passport::Smid,
                       passport::Tmid,
                       passport::PublicAnmpid,
                       passport::PublicMpid,
                       ImmutableData,
                       MutableData> AllTypes;

INSTANTIATE_TYPED_TEST_CASE_P(NoCache, DataHolderTest, AllTypes);



template<class T>
class DataHolderCacheableTest : public DataHolderTest<T> {
 protected:
  NonEmptyString GetFromCache(nfs::Message& message) {
    return this->data_holder_.template GetFromCache<T>(message);
  }
  void StoreInCache(const nfs::Message& message) {
    this->data_holder_.template StoreInCache<T>(message);
  }
};

TYPED_TEST_CASE_P(DataHolderCacheableTest);

TYPED_TEST_P(DataHolderCacheableTest, BEH_StoreInCache) {
  nfs::Message::Source source(nfs::PersonaType::kPmidAccountHolder, NodeId(NodeId::kRandomId));
  NonEmptyString content(RandomAlphaNumericString(256));
  asymm::Signature signature;
  nfs::Message message(nfs::ActionType::kPut, nfs::PersonaType::kDataHolder, source,
                       detail::DataTagValue::kAnmaidValue, Identity(RandomString(NodeId::kSize)),
                       content, signature);
  EXPECT_THROW(this->GetFromCache(message), std::system_error);
  this->StoreInCache(message);
  EXPECT_EQ(message.content(), this->GetFromCache(message));
}

REGISTER_TYPED_TEST_CASE_P(DataHolderCacheableTest, BEH_StoreInCache);

typedef testing::Types<passport::PublicAnmid,
                       passport::PublicAnsmid,
                       passport::PublicAntmid,
                       passport::PublicAnmaid,
                       passport::PublicMaid,
                       passport::PublicPmid,
                       passport::PublicAnmpid,
                       passport::PublicMpid,
                       ImmutableData,
                       MutableData> CacheableTypes;

INSTANTIATE_TYPED_TEST_CASE_P(Cache, DataHolderCacheableTest, CacheableTypes);

}  // namespace test

}  // namespace vault

}  // namespace maidsafe

