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
  nfs::Message::Destination destination(nfs::Message::Peer(nfs::PersonaType::kDataHolder,
                                                           NodeId(NodeId::kRandomId)));
  nfs::Message::Source source(nfs::Message::Peer(nfs::PersonaType::kPmidAccountHolder,
                                                 NodeId(NodeId::kRandomId)));
  NonEmptyString content(RandomAlphaNumericString(256));
  asymm::Signature signature;
  nfs::Message message(nfs::ActionType::kPut, destination, source,
                       detail::DataTagValue::kAnmaidValue, content, signature);

  std::string retrieved;
  this->HandlePutMessage(message, [&](const std::string&) {});
  this->HandleGetMessage(message,
                         [&](const std::string& data) {
                           retrieved = data;
                         });
  EXPECT_NE(retrieved.find(content.string()), -1);
}

TYPED_TEST_P(DataHolderTest, BEH_HandleGetMessage) {
  nfs::Message::Destination destination(nfs::Message::Peer(nfs::PersonaType::kDataHolder,
                                                           NodeId(NodeId::kRandomId)));
  nfs::Message::Source source(nfs::Message::Peer(nfs::PersonaType::kPmidAccountHolder,
                                                 NodeId(NodeId::kRandomId)));
  const NonEmptyString content(RandomAlphaNumericString(256));
  const asymm::Signature signature;
  nfs::Message message(nfs::ActionType::kGet, destination, source,
                       detail::DataTagValue::kAnmaidValue, content, signature);
  std::string retrieved;
  this->HandleGetMessage(message,
                         [&](const std::string& data) {
                           retrieved = data;
                          });
  EXPECT_EQ(retrieved, nfs::ReturnCode(-1).Serialise()->string());
}

TYPED_TEST_P(DataHolderTest, BEH_HandleDeleteMessage) {
  nfs::Message::Destination destination(nfs::Message::Peer(nfs::PersonaType::kDataHolder,
                                                           NodeId(NodeId::kRandomId)));
  nfs::Message::Source source(nfs::Message::Peer(nfs::PersonaType::kPmidAccountHolder,
                                                 NodeId(NodeId::kRandomId)));
  NonEmptyString content(RandomAlphaNumericString(256));
  asymm::Signature signature;
  nfs::Message message(nfs::ActionType::kPut, destination, source,
                       detail::DataTagValue::kAnmaidValue, content, signature);

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

TYPED_TEST_P(DataHolderTest, BEH_RandomAsync) {
  uint32_t events(RandomUint32() % 500);
  std::vector<nfs::Message> messages;
  std::vector<std::future<void>> future_puts, future_deletes, future_gets;

  for (uint32_t i = 0; i != events; ++i) {
    nfs::Message::Destination destination(nfs::Message::Peer(nfs::PersonaType::kDataHolder,
                                                             NodeId(NodeId::kRandomId)));
    nfs::Message::Source source(nfs::Message::Peer(nfs::PersonaType::kPmidAccountHolder,
                                                   NodeId(NodeId::kRandomId)));
    NonEmptyString content(RandomAlphaNumericString(256));
    asymm::Signature signature;
    std::string retrieved;
    nfs::Message message(nfs::ActionType::kPut, destination, source,
                         detail::DataTagValue::kAnmaidValue, content, signature);
	messages.push_back(message);

    uint32_t event(RandomUint32() % 3);
    switch (event) {
      case 0: {
        if (!messages.empty()) {
          nfs::Message message = messages[RandomUint32() % messages.size()];
          future_deletes.push_back(std::async([this, message, &retrieved] {
                                               this->HandleDeleteMessage(
                                                   message,
                                                   [&](const std::string& data) {
                                                     retrieved = data;
                                                   });
                                  }));
        } else {
          future_deletes.push_back(std::async([this, message, &retrieved] {
                                               this->HandleDeleteMessage(
                                                  message,
                                                  [&](const std::string& data) {
                                                    retrieved = data;
                                                  });
                                   }));
        }
        break;
      }
     case 1: {
        future_puts.push_back(std::async([this, message, &retrieved] {
                                          this->HandlePutMessage(
                                             message,
                                             [&](const std::string& data) {
                                               retrieved = data;
                                             });
                             }));
        break;
      }
      case 2: {
        if (!messages.empty()) {
          nfs::Message message = messages[RandomUint32() % messages.size()];
          future_gets.push_back(std::async([this, message, &retrieved] {
                                            this->HandleGetMessage(
                                               message,
                                               [&](const std::string& data) {
                                                 retrieved = data;
                                               });
                                }));
        } else {
          future_gets.push_back(std::async([this, message, &retrieved] {
                                            this->HandleGetMessage(
                                               message,
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
    EXPECT_NO_THROW(future_put.get());

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
  bool GetFromCache(nfs::Message& message) {
    return this->data_holder_.template GetFromCache<T>(message);
  }
  void StoreInCache(const nfs::Message& message) {
    this->data_holder_.template StoreInCache<T>(message);
  }
};

TYPED_TEST_CASE_P(DataHolderCacheableTest);

TYPED_TEST_P(DataHolderCacheableTest, BEH_StoreInCache) {
  nfs::Message::Destination destination(nfs::Message::Peer(nfs::PersonaType::kDataHolder,
                                                           NodeId(NodeId::kRandomId)));
  nfs::Message::Source source(nfs::Message::Peer(nfs::PersonaType::kPmidAccountHolder,
                                                 NodeId(NodeId::kRandomId)));
  NonEmptyString content(RandomAlphaNumericString(256));
  asymm::Signature signature;
  nfs::Message message(nfs::ActionType::kPut, destination, source,
                       detail::DataTagValue::kAnmaidValue, content, signature);
  EXPECT_FALSE(this->GetFromCache(message));
  this->StoreInCache(message);
  EXPECT_TRUE(this->GetFromCache(message));
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

