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
#include "maidsafe/vault/maid_account_holder/maid_account_holder.h"
#include "maidsafe/vault/metadata_manager/metadata_manager_service.h"
#include "maidsafe/vault/pmid_account_holder/pmid_account_holder.h"
#include "maidsafe/vault/data_holder/data_holder.h"

#include "maidsafe/nfs/message.h"

// TODO(Alison) - redesign tests to not use Mocks for personas

namespace maidsafe {

/*
namespace nfs {

bool operator==(const nfs::Message& lhs, const nfs::Message& rhs) {
  if (lhs.action() != rhs.action() ||
      lhs.destination_persona() != rhs.destination_persona() ||
      lhs.source_persona() != rhs.source_persona() ||
      lhs.data_type() != rhs.data_type() ||
      lhs.destination() != rhs.destination() ||
      lhs.source() != rhs.source() ||
      lhs.content() != rhs.content() ||
      lhs.signature() != rhs.signature())
    return false;
  return true;
}

}  // namespace nfs
*/

namespace vault {

/*
// TODO(Alison) - move mocks to separate file?
class MockMaidAccountHolder : public MaidAccountHolder {
 public:
  MockMaidAccountHolder();
  virtual ~MockMaidAccountHolder();

  MOCK_METHOD1(HandleMessage, void(nfs::Message message));

 private:
  MockMaidAccountHolder &operator=(const MockMaidAccountHolder&);
  MockMaidAccountHolder(const MockMaidAccountHolder&);
};

class MockMetadataManager : public MetadataManagerService {
 public:
  MockMetadataManager();
  virtual ~MockMetadataManager();

  MOCK_METHOD1(HandleMessage, void(nfs::Message message));

 private:
  MockMetadataManager &operator=(const MockMetadataManager&);
  MockMetadataManager(const MockMetadataManager&);
};

class MockPmidAccountHolder : public PmidAccountHolder {
 public:
  MockPmidAccountHolder();
  virtual ~MockPmidAccountHolder();

  MOCK_METHOD1(HandleMessage, void(nfs::Message message));

 private:
  MockPmidAccountHolder &operator=(const MockPmidAccountHolder&);
  MockPmidAccountHolder(const MockPmidAccountHolder&);
};

class MockDataHolder : public DataHolder {
 public:
  MockDataHolder();
  virtual ~MockDataHolder();

  MOCK_METHOD1(HandleMessage, void(nfs::Message message));

 private:
  MockDataHolder &operator=(const MockDataHolder&);
  MockDataHolder(const MockDataHolder&);
};
*/

namespace test {

/*
class DemultiplexerTest : public testing::Test {
 public:
  DemultiplexerTest()
      : maid_account_holder_(),
        metadata_manager_service_(),
        pmid_account_holder_(),
        data_holder_(),
        demultiplexer_(maid_account_holder_,
                       metadata_manager_service_,
                       pmid_account_holder_,
                       data_holder_) {}

  bool VerifyAndClearAllExpectations() {
    return testing::Mock::VerifyAndClearExpectations(&maid_account_holder_) &&
           testing::Mock::VerifyAndClearExpectations(&metadata_manager_service_) &&
           testing::Mock::VerifyAndClearExpectations(&pmid_account_holder_) &&
           testing::Mock::VerifyAndClearExpectations(&data_holder_);
  }

  nfs::Message GenerateValidMessage() {
    // TODO(Alison) - % 4 to match Action enum - improve?
    // TODO(Alison) - % 4 to match Persona enum - improve?
    nfs::Message message(static_cast<nfs::Action>(RandomUint32() % 4),
                         static_cast<nfs::Persona>(RandomUint32() % 4),
                         static_cast<nfs::Persona>(RandomUint32() % 4),
                         RandomUint32(),
                         NodeId(NodeId::kRandomId),
                         NodeId(NodeId::kRandomId),
                         RandomAlphaNumericString(10),
                         RandomAlphaNumericString(10));
    return message;
  }

  nfs::Message GenerateValidMessage(const nfs::Persona& dest_type) {
    // TODO(Alison) - % 4 to match Action enum - improve?
    // TODO(Alison) - % 4 to match Persona enum - improve?
    nfs::Message message(static_cast<nfs::Action>(RandomUint32() % 4),
                         dest_type,
                         static_cast<nfs::Persona>(RandomUint32() % 4),
                         RandomUint32(),
                         NodeId(NodeId::kRandomId),
                         NodeId(NodeId::kRandomId),
                         RandomAlphaNumericString(10),
                         RandomAlphaNumericString(10));
    return message;
  }

  nfs::Message GenerateValidMessage(uint16_t& expect_mah,
                                    uint16_t& expect_mdm,
                                    uint16_t& expect_pah,
                                    uint16_t& expect_dh) {
    // TODO(Alison) - % 4 to match Persona enum - improve?
    nfs::Persona dest_type(static_cast<nfs::Persona>(RandomUint32() % 4));
    switch (dest_type) {
      case nfs::Persona::kMaidAccountHolder:
        ++expect_mah;
        break;
      case nfs::Persona::kMetadataManager:
        ++expect_mdm;
        break;
      case nfs::Persona::kPmidAccountHolder:
        ++expect_pah;
        break;
      case nfs::Persona::kDataHolder:
        ++expect_dh;
        break;
      default:
        LOG(kError) << "This type of message shouldn't occur here!";
        assert(false);
        break;
    }
    return GenerateValidMessage(dest_type);
  }

  std::vector<nfs::Message> GenerateValidMessages(const uint16_t& num_messages,
                                                  uint16_t& expect_mah,
                                                  uint16_t& expect_mdm,
                                                  uint16_t& expect_pah,
                                                  uint16_t& expect_dh) {
    std::vector<nfs::Message> messages;
    if (num_messages == 0) {
      LOG(kError) << "Generated 0 messages.";
      return messages;
    }
    for (uint16_t i(0); i < num_messages; ++i) {
      messages.push_back(GenerateValidMessage(expect_mah, expect_mdm, expect_pah, expect_dh));
    }
    return messages;
  }

 protected:
  virtual void SetUp();

  virtual void TearDown() {
    EXPECT_TRUE(VerifyAndClearAllExpectations());
  }

 public:
  MockMaidAccountHolder maid_account_holder_;
  MockMetadataManager metadata_manager_service_;
  MockPmidAccountHolder pmid_account_holder_;
  MockDataHolder data_holder_;
  Demultiplexer demultiplexer_;
};

TEST_F(DemultiplexerTest, FUNC_MaidAccountHolder) {
  nfs::Message message(GenerateValidMessage(nfs::Persona::kMaidAccountHolder));

  EXPECT_CALL(maid_account_holder_, HandleMessage(message)).Times(1);
  EXPECT_CALL(metadata_manager_service_, HandleMessage(testing::_)).Times(0);
  EXPECT_CALL(pmid_account_holder_, HandleMessage(testing::_)).Times(0);
  EXPECT_CALL(data_holder_, HandleMessage(testing::_)).Times(0);

  demultiplexer_.HandleMessage(SerialiseAsString(message));
}

TEST_F(DemultiplexerTest, FUNC_MaidAccountHolderRepeat) {
  EXPECT_CALL(maid_account_holder_, HandleMessage(testing::_)).Times(100);
  EXPECT_CALL(metadata_manager_service_, HandleMessage(testing::_)).Times(0);
  EXPECT_CALL(pmid_account_holder_, HandleMessage(testing::_)).Times(0);
  EXPECT_CALL(data_holder_, HandleMessage(testing::_)).Times(0);

  for (uint16_t i(0); i < 100; ++i) {
    nfs::Message message(GenerateValidMessage(nfs::Persona::kMaidAccountHolder));
    demultiplexer_.HandleMessage(SerialiseAsString(message));
  }
}

TEST_F(DemultiplexerTest, FUNC_MetadataManager) {
  nfs::Message message(GenerateValidMessage(nfs::Persona::kMetadataManager));

  EXPECT_CALL(maid_account_holder_, HandleMessage(testing::_)).Times(0);
  EXPECT_CALL(metadata_manager_service_, HandleMessage(message)).Times(1);
  EXPECT_CALL(pmid_account_holder_, HandleMessage(testing::_)).Times(0);
  EXPECT_CALL(data_holder_, HandleMessage(testing::_)).Times(0);

  demultiplexer_.HandleMessage(SerialiseAsString(message));
}

TEST_F(DemultiplexerTest, FUNC_MetadataManagerRepeat) {
  EXPECT_CALL(maid_account_holder_, HandleMessage(testing::_)).Times(0);
  EXPECT_CALL(metadata_manager_service_, HandleMessage(testing::_)).Times(100);
  EXPECT_CALL(pmid_account_holder_, HandleMessage(testing::_)).Times(0);
  EXPECT_CALL(data_holder_, HandleMessage(testing::_)).Times(0);

  for (uint16_t i(0); i < 100; ++i) {
    nfs::Message message(GenerateValidMessage(nfs::Persona::kMetadataManager));
    demultiplexer_.HandleMessage(SerialiseAsString(message));
  }
}

TEST_F(DemultiplexerTest, FUNC_PmidAccountHolder) {
  nfs::Message message(GenerateValidMessage(nfs::Persona::kPmidAccountHolder));

  EXPECT_CALL(maid_account_holder_, HandleMessage(testing::_)).Times(0);
  EXPECT_CALL(metadata_manager_service_, HandleMessage(testing::_)).Times(0);
  EXPECT_CALL(pmid_account_holder_, HandleMessage(message)).Times(0);
  EXPECT_CALL(data_holder_, HandleMessage(testing::_)).Times(1);

  demultiplexer_.HandleMessage(SerialiseAsString(message));
}

TEST_F(DemultiplexerTest, FUNC_PmidAccountHolderRepeat) {
  EXPECT_CALL(maid_account_holder_, HandleMessage(testing::_)).Times(0);
  EXPECT_CALL(metadata_manager_service_, HandleMessage(testing::_)).Times(0);
  EXPECT_CALL(pmid_account_holder_, HandleMessage(testing::_)).Times(100);
  EXPECT_CALL(data_holder_, HandleMessage(testing::_)).Times(0);

  for (uint16_t i(0); i < 100; ++i) {
    nfs::Message message(GenerateValidMessage(nfs::Persona::kPmidAccountHolder));
    demultiplexer_.HandleMessage(SerialiseAsString(message));
  }
}

TEST_F(DemultiplexerTest, FUNC_DataHolder) {
  nfs::Message message(GenerateValidMessage(nfs::Persona::kDataHolder));

  EXPECT_CALL(maid_account_holder_, HandleMessage(testing::_)).Times(0);
  EXPECT_CALL(metadata_manager_service_, HandleMessage(testing::_)).Times(0);
  EXPECT_CALL(pmid_account_holder_, HandleMessage(testing::_)).Times(0);
  EXPECT_CALL(data_holder_, HandleMessage(message)).Times(1);

  demultiplexer_.HandleMessage(SerialiseAsString(message));
}

TEST_F(DemultiplexerTest, FUNC_DataHolderRepeat) {
  EXPECT_CALL(maid_account_holder_, HandleMessage(testing::_)).Times(0);
  EXPECT_CALL(metadata_manager_service_, HandleMessage(testing::_)).Times(0);
  EXPECT_CALL(pmid_account_holder_, HandleMessage(testing::_)).Times(0);
  EXPECT_CALL(data_holder_, HandleMessage(testing::_)).Times(100);

  for (uint16_t i(0); i < 100; ++i) {
    nfs::Message message(GenerateValidMessage(nfs::Persona::kDataHolder));
    demultiplexer_.HandleMessage(SerialiseAsString(message));
  }
}

TEST_F(DemultiplexerTest, FUNC_Scrambled) {
  EXPECT_CALL(maid_account_holder_, HandleMessage(testing::_)).Times(0);
  EXPECT_CALL(metadata_manager_service_, HandleMessage(testing::_)).Times(0);
  EXPECT_CALL(pmid_account_holder_, HandleMessage(testing::_)).Times(0);
  EXPECT_CALL(data_holder_, HandleMessage(testing::_)).Times(0);

  demultiplexer_.HandleMessage(RandomAlphaNumericString(1 + (RandomUint32() % 100)));
}

TEST_F(DemultiplexerTest, FUNC_ScrambledRepeat) {
  EXPECT_CALL(maid_account_holder_, HandleMessage(testing::_)).Times(0);
  EXPECT_CALL(metadata_manager_service_, HandleMessage(testing::_)).Times(0);
  EXPECT_CALL(pmid_account_holder_, HandleMessage(testing::_)).Times(0);
  EXPECT_CALL(data_holder_, HandleMessage(testing::_)).Times(0);

  for (uint16_t i(0); i < 100; ++i) {
    std::string scrambled_message(RandomAlphaNumericString(1 + (RandomUint32() % 100)));
    demultiplexer_.HandleMessage(scrambled_message);
  }
}

TEST_F(DemultiplexerTest, FUNC_EmptyMessage) {
  EXPECT_CALL(maid_account_holder_, HandleMessage(testing::_)).Times(0);
  EXPECT_CALL(metadata_manager_service_, HandleMessage(testing::_)).Times(0);
  EXPECT_CALL(pmid_account_holder_, HandleMessage(testing::_)).Times(0);
  EXPECT_CALL(data_holder_, HandleMessage(testing::_)).Times(0);

  std::string empty_message;
  demultiplexer_.HandleMessage(empty_message);
}

TEST_F(DemultiplexerTest, FUNC_ValidMessages) {
  std::vector<nfs::Message> messages;
  messages.push_back(GenerateValidMessage(nfs::Persona::kMaidAccountHolder));
  messages.push_back(GenerateValidMessage(nfs::Persona::kMetadataManager));
  messages.push_back(GenerateValidMessage(nfs::Persona::kPmidAccountHolder));
  messages.push_back(GenerateValidMessage(nfs::Persona::kDataHolder));

  while (messages.size() > 0) {
    uint16_t index(0);
    if (messages.size() > 1)
      index = RandomUint32() % messages.size();

    if (messages.at(index).destination_persona() == nfs::Persona::kMaidAccountHolder)
      EXPECT_CALL(maid_account_holder_, HandleMessage(messages.at(index))).Times(1);
    else
      EXPECT_CALL(maid_account_holder_, HandleMessage(testing::_)).Times(0);

    if (messages.at(index).destination_persona() == nfs::Persona::kMetadataManager)
      EXPECT_CALL(metadata_manager_service_, HandleMessage(messages.at(index))).Times(1);
    else
      EXPECT_CALL(metadata_manager_service_, HandleMessage(testing::_)).Times(0);

    if (messages.at(index).destination_persona() == nfs::Persona::kPmidAccountHolder)
      EXPECT_CALL(pmid_account_holder_, HandleMessage(messages.at(index))).Times(1);
    else
      EXPECT_CALL(pmid_account_holder_, HandleMessage(testing::_)).Times(0);

    if (messages.at(index).destination_persona() == nfs::Persona::kDataHolder)
      EXPECT_CALL(data_holder_, HandleMessage(messages.at(index))).Times(1);
    else
      EXPECT_CALL(data_holder_, HandleMessage(testing::_)).Times(0);

    demultiplexer_.HandleMessage(SerialiseAsString(messages.at(index)));
    EXPECT_TRUE(VerifyAndClearAllExpectations());
    messages.erase(messages.begin() + index);
  }
}

TEST_F(DemultiplexerTest, FUNC_ValidMessagesRepeat) {
  uint16_t expect_mah(0),
           expect_mdm(0),
           expect_pah(0),
           expect_dh(0);
  std::vector<nfs::Message> messages(GenerateValidMessages(100,
                                                           expect_mah,
                                                           expect_mdm,
                                                           expect_pah,
                                                           expect_dh));

  EXPECT_CALL(maid_account_holder_, HandleMessage(testing::_)).Times(expect_mah);
  EXPECT_CALL(metadata_manager_service_, HandleMessage(testing::_)).Times(expect_mdm);
  EXPECT_CALL(pmid_account_holder_, HandleMessage(testing::_)).Times(expect_pah);
  EXPECT_CALL(data_holder_, HandleMessage(testing::_)).Times(expect_dh);

  for (auto message : messages)
    demultiplexer_.HandleMessage(SerialiseAsString(message));
}

TEST_F(DemultiplexerTest, FUNC_ValidMessagesParallel) {
  uint16_t num_messages = 4 + (RandomUint32() % 7);
  uint16_t expect_mah(0),
           expect_mdm(0),
           expect_pah(0),
           expect_dh(0);

  std::vector<nfs::Message> messages(GenerateValidMessages(num_messages,
                                                           expect_mah,
                                                           expect_mdm,
                                                           expect_pah,
                                                           expect_dh));

  EXPECT_CALL(maid_account_holder_, HandleMessage(testing::_)).Times(expect_mah);
  EXPECT_CALL(metadata_manager_service_, HandleMessage(testing::_)).Times(expect_mdm);
  EXPECT_CALL(pmid_account_holder_, HandleMessage(testing::_)).Times(expect_pah);
  EXPECT_CALL(data_holder_, HandleMessage(testing::_)).Times(expect_dh);

  std::vector<boost::thread> threads(num_messages);
  for (uint16_t i(0); i < num_messages; ++i) {
    threads[i] = boost::thread([&] {
                               demultiplexer_.HandleMessage(SerialiseAsString(messages[i]));
                 });
  }
  for (boost::thread& thread : threads)
    thread.join();
}

TEST_F(DemultiplexerTest, FUNC_MixedMessagesRepeat) {
  uint16_t num_messages(100);
  uint16_t expect_mah(0),
           expect_mdm(0),
           expect_pah(0),
           expect_dh(0);

  std::vector<nfs::Message> messages(GenerateValidMessages(num_messages,
                                                           expect_mah,
                                                           expect_mdm,
                                                           expect_pah,
                                                           expect_dh));

  EXPECT_CALL(maid_account_holder_, HandleMessage(testing::_)).Times(expect_mah);
  EXPECT_CALL(metadata_manager_service_, HandleMessage(testing::_)).Times(expect_mdm);
  EXPECT_CALL(pmid_account_holder_, HandleMessage(testing::_)).Times(expect_pah);
  EXPECT_CALL(data_holder_, HandleMessage(testing::_)).Times(expect_dh);

  for (uint16_t i(0); i < num_messages; ++i) {
    if (i % 13 == 0 || i % 19 == 0) {
      std::string bad_message(RandomAlphaNumericString(1 + RandomUint32() % 50));
      demultiplexer_.HandleMessage(bad_message);
    }
    demultiplexer_.HandleMessage(SerialiseAsString(messages.at(i)));
  }
}

TEST_F(DemultiplexerTest, FUNC_MixedMessagesParallel) {
  uint16_t num_messages = 4 + (RandomUint32() % 7);
  uint16_t expect_mah(0),
           expect_mdm(0),
           expect_pah(0),
           expect_dh(0);

  std::vector<nfs::Message> messages(GenerateValidMessages(num_messages,
                                                           expect_mah,
                                                           expect_mdm,
                                                           expect_pah,
                                                           expect_dh));

  EXPECT_CALL(maid_account_holder_, HandleMessage(testing::_)).Times(expect_mah);
  EXPECT_CALL(metadata_manager_service_, HandleMessage(testing::_)).Times(expect_mdm);
  EXPECT_CALL(pmid_account_holder_, HandleMessage(testing::_)).Times(expect_pah);
  EXPECT_CALL(data_holder_, HandleMessage(testing::_)).Times(expect_dh);

  std::vector<boost::thread> threads(num_messages);
  std::vector<boost::thread> bad_threads;
  for (uint16_t i(0); i < num_messages; ++i) {
    if (i % 13 == 0 || 1 % 19 == 0) {
      std::string bad_message(RandomAlphaNumericString(1 + RandomUint32() % 50));
      bad_threads.push_back(boost::thread([&] {
                              demultiplexer_.HandleMessage(bad_message);
                            }));
    }
    threads[i] = boost::thread([&] {
                               demultiplexer_.HandleMessage(nfs::SerialiseAsString(messages[i]));
                 });
  }
  for (boost::thread& thread : bad_threads)
    thread.join();
  for (boost::thread& thread : threads)
    thread.join();
}

TEST_F(DemultiplexerTest, FUNC_BadMessageHaveCache) {
  std::string bad_message(RandomAlphaNumericString(1 + RandomUint32() % 50));
  std::string bad_message_passed(bad_message);
  EXPECT_FALSE(demultiplexer_.HaveCache(bad_message_passed));
  EXPECT_EQ(bad_message, bad_message_passed);
}

TEST_F(DemultiplexerTest, FUNC_ValidMessageHaveCache) {
  nfs::Message message(GenerateValidMessage());
  std::string string(SerialiseAsString(message));
  std::string passed_string(string);
  EXPECT_FALSE(demultiplexer_.HaveCache(passed_string));
  EXPECT_EQ(string, passed_string);
}

TEST_F(DemultiplexerTest, FUNC_StoreCacheHaveCache) {
  for (uint16_t i(0); i < 20; ++i) {
    nfs::Message message(GenerateValidMessage());
    std::string string(SerialiseAsString(message));
    demultiplexer_.StoreCache(string);
    std::string passed_string(string);
    if (message.destination_persona() == nfs::Persona::kDataHolder) {
      EXPECT_TRUE(demultiplexer_.HaveCache(passed_string));
      // TODO(Alison) - relationship between string and passed string?
    } else {
      EXPECT_FALSE(demultiplexer_.HaveCache(passed_string));
      EXPECT_EQ(string, passed_string);
    }
  }
}

TEST_F(DemultiplexerTest, FUNC_RepeatStoreCache) {
  nfs::Message message(GenerateValidMessage(nfs::Persona::kDataHolder));
  std::string string(SerialiseAsString(message));
  std::string passed_string(string);

  demultiplexer_.StoreCache(string);
  EXPECT_TRUE(demultiplexer_.HaveCache(passed_string));
  // TODO(Alison) - relationship between string and passed string?

  passed_string = string;
  demultiplexer_.StoreCache(string);
  EXPECT_TRUE(demultiplexer_.HaveCache(passed_string));
  // TODO(Alison) - relationship between string and passed string?
}

TEST_F(DemultiplexerTest, FUNC_RetrieveOldCache) {
  int buffer_size(10);  // TODO(Alison) - get real buffer size
  nfs::Message message(GenerateValidMessage(nfs::Persona::kDataHolder));
  std::string string(SerialiseAsString(message));
  std::string passed_string(string);
  demultiplexer_.StoreCache(string);
  EXPECT_TRUE(demultiplexer_.HaveCache(passed_string));
  // TODO(Alison) - relationship between string and passed string?

  for (uint16_t i(0); i < buffer_size; ++i) {
    std::string temp_string(SerialiseAsString(GenerateValidMessage(nfs::Persona::kDataHolder)));
    demultiplexer_.StoreCache(temp_string);
    std::string temp_passed_string(temp_string);
    EXPECT_TRUE(demultiplexer_.HaveCache(temp_passed_string));
    // TODO(Alison) - relationship between string and passed string?
  }

  // expect original message to be pushed out of buffer
  passed_string = string;
  EXPECT_FALSE(demultiplexer_.HaveCache(passed_string));
  EXPECT_EQ(string, passed_string);
}
*/

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
