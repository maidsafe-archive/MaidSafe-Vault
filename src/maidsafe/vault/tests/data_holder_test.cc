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

#include "maidsafe/vault/data_holder/data_holder.h"

#include <memory>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/common/test.h"

#include "maidsafe/nfs/data_message.h"


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
  void HandlePutMessage(const nfs::DataMessage& data_message,
                        const routing::ReplyFunctor& reply_functor) {
    data_holder_.HandlePutMessage<T>(data_message, reply_functor);
  }
  void HandleGetMessage(const nfs::DataMessage& data_message,
                        const routing::ReplyFunctor& reply_functor) {
    data_holder_.HandleGetMessage<T>(data_message, reply_functor);
  }
  void HandleDeleteMessage(const nfs::DataMessage& data_message,
                           const routing::ReplyFunctor& reply_functor) {
    data_holder_.HandleDeleteMessage<T>(data_message, reply_functor);
  }

  maidsafe::test::TestPath vault_root_directory_;
  DataHolder data_holder_;
};

TYPED_TEST_CASE_P(DataHolderTest);

TYPED_TEST_P(DataHolderTest, BEH_HandlePutMessage) {
  nfs::MessageSource source(nfs::PersonaType::kPmidAccountHolder, NodeId(NodeId::kRandomId));
  NonEmptyString content(RandomAlphaNumericString(256));
  nfs::DataMessage::Data data(detail::DataTagValue::kAnmaidValue,
                              Identity(RandomString(NodeId::kSize)), content, RandomInt32());
  nfs::DataMessage data_message(nfs::DataMessage::ActionType::kPut, nfs::PersonaType::kDataHolder,
                                source, data);

  std::string retrieved;
  this->HandlePutMessage(data_message, [&](const std::string&) {});
  this->HandleGetMessage(data_message,
                         [&](const std::string& data) {
                           retrieved = data;
                         });
  EXPECT_NE(retrieved.find(content.string()), -1);
}

TYPED_TEST_P(DataHolderTest, BEH_HandleGetMessage) {
  nfs::MessageSource source(nfs::PersonaType::kPmidAccountHolder, NodeId(NodeId::kRandomId));
  const NonEmptyString content(RandomAlphaNumericString(256));
  nfs::DataMessage::Data data(detail::DataTagValue::kAnmaidValue,
                              Identity(RandomString(NodeId::kSize)), content, RandomInt32());
  nfs::DataMessage data_message(nfs::DataMessage::ActionType::kGet, nfs::PersonaType::kDataHolder,
                                source, data);
  std::string retrieved;
  this->HandleGetMessage(data_message,
                         [&](const std::string& data) {
                           retrieved = data;
                          });
  EXPECT_EQ(retrieved, nfs::ReturnCode(-1).Serialise()->string());
}

TYPED_TEST_P(DataHolderTest, BEH_HandleDeleteMessage) {
  nfs::MessageSource source(nfs::PersonaType::kPmidAccountHolder, NodeId(NodeId::kRandomId));
  NonEmptyString content(RandomAlphaNumericString(256));
  nfs::DataMessage::Data data(detail::DataTagValue::kAnmaidValue,
                              Identity(RandomString(NodeId::kSize)), content, RandomInt32());
  nfs::DataMessage data_message(nfs::DataMessage::ActionType::kPut, nfs::PersonaType::kDataHolder,
                                source, data);

  std::string retrieved;
  this->HandlePutMessage(data_message, [&](const std::string&) {});
  this->HandleGetMessage(data_message,
                         [&](const std::string& data) {
                           retrieved = data;
                          });
  EXPECT_NE(retrieved.find(content.string()), -1);
  this->HandleDeleteMessage(data_message,
                            [&](const std::string& data) {
                              retrieved = data;
                            });
  EXPECT_EQ(retrieved, nfs::ReturnCode(0).Serialise()->string());
  this->HandleGetMessage(data_message,
                         [&](const std::string& data) {
                           retrieved = data;
                         });
  EXPECT_EQ(retrieved, nfs::ReturnCode(-1).Serialise()->string());
}

TYPED_TEST_P(DataHolderTest, BEH_RandomAsync) {
  uint32_t events(RandomUint32() % 100);
  std::vector<nfs::DataMessage> data_messages;
  std::vector<std::future<void>> future_puts, future_deletes, future_gets;

  for (uint32_t i = 0; i != events; ++i) {
    nfs::MessageSource source(nfs::PersonaType::kPmidAccountHolder, NodeId(NodeId::kRandomId));
    NonEmptyString content(RandomAlphaNumericString(256));
    std::string retrieved;
    nfs::DataMessage::Data data(detail::DataTagValue::kAnmaidValue,
                                Identity(RandomString(NodeId::kSize)), content, RandomInt32());
    nfs::DataMessage data_message(nfs::DataMessage::ActionType::kPut, nfs::PersonaType::kDataHolder,
                                  source, data);
    data_messages.push_back(data_message);

    uint32_t event(RandomUint32() % 3);
    switch (event) {
      case 0: {
        if (!data_messages.empty()) {
          nfs::DataMessage data_msg = data_messages[RandomUint32() % data_messages.size()];
          future_deletes.push_back(std::async([this, data_msg, &retrieved] {
                                               this->HandleDeleteMessage(
                                                   data_msg,
                                                   [&](const std::string& /*data*/) {
                                                   });
                                  }));
        } else {
          future_deletes.push_back(std::async([this, data_message, &retrieved] {
                                               this->HandleDeleteMessage(
                                                  data_message,
                                                  [&](const std::string& data) {
                                                    retrieved = data;
                                                  });
                                   }));
        }
        break;
      }
     case 1: {
        future_puts.push_back(std::async([this, data_message, &retrieved] {
                                          this->HandlePutMessage(
                                             data_message,
                                             [&](const std::string& /*data*/) {
                                             });
                             }));
        break;
      }
      case 2: {
        if (!data_messages.empty()) {
          nfs::DataMessage data_msg = data_messages[RandomUint32() % data_messages.size()];
          future_gets.push_back(std::async([this, data_msg, &retrieved] {
                                            this->HandleGetMessage(
                                               data_msg,
                                               [&](const std::string& data) {
                                                 assert(!data.empty());
                                               });
                                }));
        } else {
          future_gets.push_back(std::async([this, data_message, &retrieved] {
                                            this->HandleGetMessage(
                                               data_message,
                                               [&](const std::string& data) {
                                                 retrieved = data;
                                               });
                                          }));
        }
        break;
      }
    }
  }

  for (auto& future_put : future_puts)
    future_put.get();

  for (auto& future_delete : future_deletes) {
    try {
      future_delete.get();
    }
    catch(const std::exception& e) {
      std::string msg(e.what());
      LOG(kError) << msg;
    }
  }

  for (auto& future_get : future_gets) {
    try {
      future_get.get();
    }
    catch(const std::exception& e) {
      std::string msg(e.what());
      LOG(kError) << msg;
    }
  }
}

REGISTER_TYPED_TEST_CASE_P(DataHolderTest,
                           BEH_HandlePutMessage,
                           BEH_HandleGetMessage,
                           BEH_HandleDeleteMessage,
                           BEH_RandomAsync);

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
  NonEmptyString GetFromCache(nfs::DataMessage& data_message) {
    return this->data_holder_.template GetFromCache<T>(data_message);
  }
  void StoreInCache(const nfs::DataMessage& data_message) {
    this->data_holder_.template StoreInCache<T>(data_message);
  }
};

TYPED_TEST_CASE_P(DataHolderCacheableTest);

TYPED_TEST_P(DataHolderCacheableTest, BEH_StoreInCache) {
  nfs::MessageSource source(nfs::PersonaType::kPmidAccountHolder, NodeId(NodeId::kRandomId));
  NonEmptyString content(RandomAlphaNumericString(256));
  nfs::DataMessage::Data data(detail::DataTagValue::kAnmaidValue,
                              Identity(RandomString(NodeId::kSize)), content, RandomInt32());
  nfs::DataMessage data_message(nfs::DataMessage::ActionType::kPut, nfs::PersonaType::kDataHolder,
                                source, data);
  EXPECT_THROW(this->GetFromCache(data_message), std::system_error);
  this->StoreInCache(data_message);
  EXPECT_EQ(data_message.data().content, this->GetFromCache(data_message));
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

