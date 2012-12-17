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

#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/common/test.h"

#include "maidsafe/vault/demultiplexer.h"


namespace maidsafe {

namespace vault {

namespace test {

class Demultiplexer;  // TODO(Alison) - include real Demultiplexer

namespace protobuf {  // TODO(Alison) - include real protobuf etc.
  class Message;
  enum kPersonaType {
    MaidAccoutHolder,
    MetaDataManager,
    PmidAccountHolder,
    DataHolder
  };
}

class Object {  // TODO(Alison) - replace this with actual object
 public:
  Object();
  virtual ~Object();
  virtual void HandleMessage(protobuf::Message message);
};

class MockObject : public Object {  // TODO(Alison) - move this to separate file?
 public:
  MockObject();
  virtual ~MockObject();

  MOCK_METHOD1(HandleMessage, void(protobuf::Message message));

 private:
  MockObject &operator=(const MockObject&);
  MockObject(const MockObject&);
};


class DemultiplexerTest : public testing::Test {
 public:
  DemultiplexerTest()
      : maid_account_holder_(),
        meta_data_manager_(),
        pmid_account_holder_(),
        data_holder_(),
        demultiplexer_(maid_account_holder_,
                       meta_data_manager_,
                       pmid_account_holder_,
                       data_holder_) {}

  bool VerifyAndClearAllExpectations() {
    return testing::Mock::VerifyAndClearExpectations(maid_account_holder_) &&
           testing::Mock::VerifyAndClearExpectations(meta_data_manager_) &&
           testing::Mock::VerifyAndClearExpectations(pmid_account_holder_) &&
           testing::Mock::VerifyAndClearExpectations(data_holder_);
  }

  protobuf::Message GenerateValidMessage(const protobuf::kPersonaType& type) {
    protobuf::Message message(GenerateTypelessMessage());
    message.set_identity_type(type);
    return message;
  }

  protobuf::Message GenerateTypelessMessage() {
    protobuf::Message message;
    // TODO(Alison) - % 4 to match kActionType enum - improve?
    message.set_action_type(RandomUint32() % 4);
    // TODO(Alison) - % 3 to match kDataType enum - improve?
    message.set_data_type(RandomUint32() % 3);
    message.set_destination(RandomAlphaNumericString(10));
    message.set_destination(RandomAlphaNumericString(10));
    message.set_destination(RandomAlphaNumericString(10));
    message.set_destination(RandomAlphaNumericString(10));
    return message;
  }

  protobuf::Message GenerateValidMessage(uint16_t& expect_mah,
                                         uint16_t& expect_mdm,
                                         uint16_t& expect_pah,
                                         uint16_t& expect_dh) {
    // TODO(Alison) - % 4 to match kPersonaType enum - improve?
    protobuf::kPersonaType type(static_cast<protobuf::kPersonaType>(RandomUint32() % 4));
    switch (type) {
      case protobuf::kPersonaType::MaidAccountHolder:
        ++expect_mah;
        break;
      case protobuf::kPersonaType::MetaDataManager:
        ++expect_mdm;
        break;
      case protobuf::kPersonaType::PmidAccountHolder:
        ++expect_pah;
        break;
      case profobuf::kPersonaType::DataHolder:
        ++expect_dh;
        break;
      case default:
        ASSERT_TRUE(false) << "This type of message shouldn't occur here!";
        break;
    }
    return GenerateValidMessage(type);
  }

  std::vector<protobuf::Message> GenerateValidMessages(const uint16_t& num_messages,
                                                       uint16_t& expect_mah,
                                                       uint16_t& expect_mdm,
                                                       uint16_t& expect_pah,
                                                       uint16_t& expect_dh) {
    std::vector<protobuf::Message> messages;
    if (num_messages == 0) {
      LOG(kError) << "Generated 0 messages.";
      return messages;
    }
    for (uint16_t i(0); i < num_messages; ++i) {
      messages.push_back(GenerateValidMessage(expect_mah, expect_mdm, expect_pah, expect_dh));
    }
    return messages;
  }

  std::vector<protobuf::Message> GenerateMixedMessages(const uint16_t& num_messages,
                                                       uint16_t& expect_mah,
                                                       uint16_t& expect_mdm,
                                                       uint16_t& expect_pah,
                                                       uint16_t& expect_dh) {
    std::vector<protobuf::Message> messages;
    if (num_messages == 0) {
      LOG(kError) << "Generated 0 messages.";
      return messages;
    }
    for (uint16_t i(0); i < 100; ++i) {
      // TODO(Alison) - currently approx 1/20 messages will be typeless - allow more flexibility?
      if (RandomUint32() % 20 != 0) {
        messages.push_back(GenerateValidMessage(expect_mah, expect_mdm, expect_pah, expect_dh));
      } else {
        messages.push_back(GenerateTypelessMessage());
      }
    }
    return messages;
  }

 protected:
  virtual void SetUp();

  virtual void TearDown() {
    EXPECT_TRUE(VerifyAndClearAllExpectations());
  }

 private:
  MockObject maid_account_holder_;
  MockObject meta_data_manager_;
  MockObject pmid_account_holder_;
  MockObject data_holder_;
  Demultiplexer demultiplexer_;
}

TEST_F(DemultiplexerTest, FUNC_MaidAccountHolder) {
  protobuf::Message message(GenerateValidMessage(protobuf::kPersonaType::MaidAccountHolder));

  EXPECT_CALL(maid_account_holder_, HandleMessage(message)).Times(1);
  EXPECT_CALL(meta_data_manager_, HandleMessage()).Times(0);
  EXPECT_CALL(pmid_account_holder_, HandleMessage()).Times(0);
  EXPECT_CALL(data_holder_, HandleMessage()).Times(0);

  demultiplexer_.HandleMessage(message);
}

TEST_F(DemultiplexerTest, FUNC_MaidAccountHolderRepeat) {
  EXPECT_CALL(maid_account_holder_, HandleMessage()).Times(100);
  EXPECT_CALL(meta_data_manager_, HandleMessage()).Times(0);
  EXPECT_CALL(pmid_account_holder_, HandleMessage()).Times(0);
  EXPECT_CALL(data_holder_, HandleMessage()).Times(0);

  for (uint16_t i(0); i < 100; ++i) {
    protobuf::Message message(GenerateValidMessage(protobuf::kPersonaType::MaidAccountHolder));
    demultiplexer_.HandleMessage(message);
  }
}

TEST_F(DemultiplexerTest, FUNC_MetaDataManager) {
  protobuf::Message message(GenerateValidMessage(protobuf::kPersonaType::MetaDataManager));

  EXPECT_CALL(maid_account_holder_, HandleMessage()).Times(0);
  EXPECT_CALL(meta_data_manager_, HandleMessage(message)).Times(1);
  EXPECT_CALL(pmid_account_holder_, HandleMessage()).Times(0);
  EXPECT_CALL(data_holder_, HandleMessage()).Times(0);

  demultiplexer_.HandleMessage(message);
}

TEST_F(DemultiplexerTest, FUNC_MetaDataManagerRepeat) {
  EXPECT_CALL(maid_account_holder_, HandleMessage()).Times(0);
  EXPECT_CALL(meta_data_manager_, HandleMessage()).Times(100);
  EXPECT_CALL(pmid_account_holder_, HandleMessage()).Times(0);
  EXPECT_CALL(data_holder_, HandleMessage()).Times(0);

  for (uint16_t i(0); i < 100; ++i) {
    protobuf::Message message(GenerateValidMessage(protobuf::kPersonaType::MetaDataManager));
    demultiplexer_.HandleMessage(message);
  }
}

TEST_F(DemultiplexerTest, FUNC_PmidAccountHolder) {
  protobuf::Message message(GenerateValidMessage(protobuf::kPersonaType::PmidAccountHolder));

  EXPECT_CALL(maid_account_holder_, HandleMessage()).Times(0);
  EXPECT_CALL(meta_data_manager_, HandleMessage()).Times(0);
  EXPECT_CALL(pmid_account_holder_, HandleMessage(message)).Times(0);
  EXPECT_CALL(data_holder_, HandleMessage()).Times(1);

  demultiplexer_.HandleMessage(message);
}

TEST_F(DemultiplexerTest, FUNC_PmidAccountHolderRepeat) {
  EXPECT_CALL(maid_account_holder_, HandleMessage()).Times(0);
  EXPECT_CALL(meta_data_manager_, HandleMessage()).Times(0);
  EXPECT_CALL(pmid_account_holder_, HandleMessage()).Times(100);
  EXPECT_CALL(data_holder_, HandleMessage()).Times(0);

  for (uint16_t i(0); i < 100; ++i) {
    protobuf::Message message(GenerateValidMessage(protobuf::kPersonaType::PmidAccountHolder));
    demultiplexer_.HandleMessage(message);
  }
}

TEST_F(DemultiplexerTest, FUNC_DataHolder) {
  protobuf::Message message(GenerateValidMessage(protobuf::kPersonaType::DataHolder));

  EXPECT_CALL(maid_account_holder_, HandleMessage()).Times(0);
  EXPECT_CALL(meta_data_manager_, HandleMessage()).Times(0);
  EXPECT_CALL(pmid_account_holder_, HandleMessage()).Times(0);
  EXPECT_CALL(data_holder_, HandleMessage(message)).Times(1);

  demultiplexer_.HandleMessage(message);
}

TEST_F(DemultiplexerTest, FUNC_DataHolderRepeat) {
  EXPECT_CALL(maid_account_holder_, HandleMessage()).Times(0);
  EXPECT_CALL(meta_data_manager_, HandleMessage()).Times(0);
  EXPECT_CALL(pmid_account_holder_, HandleMessage()).Times(0);
  EXPECT_CALL(data_holder_, HandleMessage()).Times(100);

  for (uint16_t i(0); i < 100; ++i) {
    protobuf::Message message(GenerateValidMessage(protobuf::kPersonaType::DataHolder));
    demultiplexer_.HandleMessage(message);
  }
}

TEST_F(DemultiplexerTest, FUNC_Typeless) {
  protobuf::Message message(GenerateTypelessMessage());

  EXPECT_CALL(maid_account_holder_, HandleMessage()).Times(0);
  EXPECT_CALL(meta_data_manager_, HandleMessage()).Times(0);
  EXPECT_CALL(pmid_account_holder_, HandleMessage()).Times(0);
  EXPECT_CALL(data_holder_, HandleMessage()).Times(0);

  demultiplexer_.HandleMessage(message);
}

TEST_F(DemultiplexerTest, FUNC_TypelessRepeat) {
  EXPECT_CALL(maid_account_holder_, HandleMessage()).Times(0);
  EXPECT_CALL(meta_data_manager_, HandleMessage()).Times(0);
  EXPECT_CALL(pmid_account_holder_, HandleMessage()).Times(0);
  EXPECT_CALL(data_holder_, HandleMessage()).Times(0);

  for (uint16_t i(0); i < 100; ++i) {
    protobuf::Message message(GenerateTypelessMessage());
    demultiplexer_.HandleMessage(message);
  }
}

TEST_F(DemultiplexerTest, FUNC_EmptyMessage) {
  EXPECT_CALL(maid_account_holder_, HandleMessage()).Times(0);
  EXPECT_CALL(meta_data_manager_, HandleMessage()).Times(0);
  EXPECT_CALL(pmid_account_holder_, HandleMessage()).Times(0);
  EXPECT_CALL(data_holder_, HandleMessage()).Times(0);

  protobuf::Message message;
  demultiplexer_.HandleMessage(message);
}

TEST_F(DemultiplexerTest, FUNC_ValidMessages) {
  std::vector<protobuf::Message> messages;
  messages.push_back(GenerateValidMessage(protobuf::kPersonaType::MaidAccountHolder));
  messages.push_back(GenerateValidMessage(protobuf::kPersonaType::MetaDataManager));
  messages.push_back(GenerateValidMessage(protobuf::kPersonaType::PmidAccountHolder));
  messages.push_back(GenerateValidMessage(profobuf::kPersonaType::DataHolder));

  while (messages.size() > 0) {
    uint16_t index(0);
    if (messages.size() > 1)
      index = RandomUint32() % messages.size();

    if (messages.at(index).identity_type() == protobuf::kPersonaType::MaidAccountHolder)
      EXPECT_CALL(maid_account_holder_, HandleMessage(messages.at(index))).Times(1);
    else
      EXPECT_CALL(maid_account_holder_, HandleMessage()).Times(0);

    if (messages.at(index).identity_type() == protobuf::kPersonaType::MetaDataManager)
      EXPECT_CALL(meta_data_manager_, HandleMessage(messages.at(index))).Times(1);
    else
      EXPECT_CALL(meta_data_manager_, HandleMessage()).Times(0);

    if (messages.at(index).identity_type() == protobuf::kPersonaType::PmidAccountHolder)
      EXPECT_CALL(pmid_account_holder_, HandleMessage(messages.at(index))).Times(1);
    else
      EXPECT_CALL(pmid_account_holder_, HandleMessage()).Times(0);

    if (messages.at(index).identity_type() == profobuf::kPersonaType::DataHolder)
      EXPECT_CALL(data_holder_, HandleMessage(messages.at(index))).Times(1);
    else
      EXPECT_CALL(data_holder_, HandleMessage()).Times(0);

    demultiplexer_.HandleMessage(messages.at(index));
    EXPECT_TRUE(VerifyAndClearAllExpectations());
    messages.erase(messages.begin() + index);
  }
}

TEST_F(DemultiplexerTest, FUNC_ValidMessagesRepeat) {
  uint16_t expect_mah(0),
           expect_mdm(0),
           expect_pah(0),
           expect_dh(0);
  std::vector<protobuf::Message> messages(GenerateValidMessages(100,
                                                                expect_mah,
                                                                expect_mdm,
                                                                expect_pah,
                                                                expect_dh))

  EXPECT_CALL(maid_account_holder_, HandleMessage()).Times(expect_mah);
  EXPECT_CALL(meta_data_manager_, HandleMessage()).Times(expect_mdm);
  EXPECT_CALL(pmid_account_holder_, HandleMessage()).Times(expect_pah);
  EXPECT_CALL(data_holder_, HandleMessage()).Times(expect_dh);

  for (auto message : messages)
    demultiplexer_.HandleMessage(message);
}

TEST_F(DemultiplexerTest, FUNC_ValidMessagesParallel) {
  uint16_t expect_mah(0),
           expect_mdm(0),
           expect_pah(0),
           expect_dh(0);

  std::vector<protobuf::Message> messages(GenerateValidMessages(100,
                                                                expect_mah,
                                                                expect_mdm,
                                                                expect_pah,
                                                                expect_dh));

  EXPECT_CALL(maid_account_holder_, HandleMessage()).Times(expect_mah);
  EXPECT_CALL(meta_data_manager_, HandleMessage()).Times(expect_mdm);
  EXPECT_CALL(pmid_account_holder_, HandleMessage()).Times(expect_pah);
  EXPECT_CALL(data_holder_, HandleMessage()).Times(expect_dh);

  std::vector<boost::thread> threads;
    for (auto message : messages) {
    threads.push_back(boost::thread([&] { demultiplexer_.HandleMessage(message); }));
  }
  for (auto thread : threads)
    thread.join();
}

TEST_F(DemultiplexerTest, FUNC_MixedMessagesRepeat) {
  uint16_t expect_mah(0),
           expect_mdm(0),
           expect_pah(0),
           expect_dh(0);

  std::vector<protobuf::Messages> messages(GenerateMixedMessages(100,
                                                                 expect_mah,
                                                                 expect_mdm,
                                                                 expect_pah,
                                                                 expect_dh));

  EXPECT_CALL(maid_account_holder_, HandleMessage()).Times(expect_mah);
  EXPECT_CALL(meta_data_manager_, HandleMessage()).Times(expect_mdm);
  EXPECT_CALL(pmid_account_holder_, HandleMessage()).Times(expect_pah);
  EXPECT_CALL(data_holder_, HandleMessage()).Times(expect_dh);

  for (auto message : messages)
    demultiplexer_.HandleMessage(message);
}

TEST_F(DemultiplexerTest, FUNC_MixedMessagesParallel) {
  uint16_t expect_mah(0),
           expect_mdm(0),
           expect_pah(0),
           expect_dh(0);

  std::vector<protobuf::Message> messages(GenerateMixedMessages(100,
                                                                expect_mah,
                                                                expect_mdm,
                                                                expect_pah,
                                                                expect_dh));

  EXPECT_CALL(maid_account_holder_, HandleMessage()).Times(expect_mah);
  EXPECT_CALL(meta_data_manager_, HandleMessage()).Times(expect_mdm);
  EXPECT_CALL(pmid_account_holder_, HandleMessage()).Times(expect_pah);
  EXPECT_CALL(data_holder_, HandleMessage()).Times(expect_dh);

  std::vector<boost::thread> threads;
    for (auto message : messages) {
    threads.push_back(boost::thread([&] { demultiplexer_.HandleMessage(message); }));
  }
  for (auto thread : threads)
    thread.join();
}

// TODO(Alison) - add tests for caching

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
