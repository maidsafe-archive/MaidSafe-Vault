// /*******************************************************************************
// *  Copyright 2012 maidsafe.net limited                                        *
// *                                                                             *
// *  The following source code is property of maidsafe.net limited and is not   *
// *  meant for external use.  The use of this code is governed by the licence   *
// *  file licence.txt found in the root of this directory and also on           *
// *  www.maidsafe.net.                                                          *
// *                                                                             *
// *  You are not free to copy, amend or otherwise use this source code without  *
// *  the explicit written permission of the board of directors of maidsafe.net. *
// ******************************************************************************/

//#include <memory>
//#include "boost/filesystem/path.hpp"

//#include "maidsafe/common/log.h"
//#include "maidsafe/common/utils.h"
//#include "maidsafe/common/test.h"

//#include "maidsafe/routing/routing_api.h"
//#include "maidsafe/vault/data_holder.h"

//namespace maidsafe {

//namespace vault {

//namespace test {

//class DataHolderTest : public testing::Test {
// public:
//  DataHolderTest()
//      : vault_root_directory_("vault-root-directory") {
//    auto pmid(passport::Pmid(MakeMaid()));
//    routing_.reset(new Routing(&pmid));
//    data_holder_.reset(routing_, vault_root_directory_);
//  }

// protected:
//  std::shared_ptr<routing::Routing> routing_;
//  std::shared_ptr<DataHolder> data_holder_;
//  boost::filesystem::path vault_root_directory_;
//};

//TEST(DataHolderTest, BEH_HandleMessage) {
//  NodeId destination(this->routing_->kNodeId()), source(NodeId::kRandomId);
//  NonEmptyString content(RandomAlphaNumericString(256));
//  asymm::Signature signature;
//  nfs::Message message(nfs::ActionType::kPut,
//                       nfs::PersonaType::kDataHolder,
//                       nfs::PersonaType::kPmidAccountHolder,
//                       0,
//                       destination,
//                       source,
//                       content,
//                       signature);
//  this->data_holder_->HandleMessage(message, [&](const std::string&) {});
//  nfs::Message message(nfs::ActionType::kGet,
//                       nfs::PersonaType::kDataHolder,
//                       nfs::PersonaType::kPmidAccountHolder,
//                       0,
//                       destination,
//                       source,
//                       content,
//                       signature);
//  std::string retrieved;
//  this->data_holder_->HandleMessage(message,
//                                    [retrieved](const std::string& string) {
//                                      retrieved = string;
//                                    });
//EXPECT_EQ(retrieved, content);
//}

//TEST(DataHolderTest, BEH_HaveCache) {
//  NodeId destination(this->routing_->kNodeId()), source(NodeId::kRandomId);
//  NonEmptyString content(RandomAlphaNumericString(256));
//  asymm::Signature signature;
//  nfs::Message message(nfs::ActionType::kPut,
//                       nfs::PersonaType::kDataHolder,
//                       nfs::PersonaType::kPmidAccountHolder,
//                       0,
//                       destination,
//                       source,
//                       content,
//                       signature);
//  EXPECT_TRUE(this->data_holder_->HaveCache(message));
//}

//TEST(DataHolderTest, BEH_StoreCache) {
//}

//TEST(DataHolderTest, BEH_StopSending) {
//  data_holder_->StopSending();
//  EXPECT_TRUE(data_holder_->stop_sending_, true);
//}

//TEST(DataHolderTest, BEH_ResumeSending) {
//  data_holder_->StopSending();
//  EXPECT_TRUE(data_holder_->stop_sending_, true);
//  data_holder_->ResumeSending();
//  EXPECT_TRUE(data_holder_->stop_sending_, false);
//}

//TEST(DataHolderTest, BEH_HandlePutMessage) {
//  NodeId destination(this->routing_->kNodeId()), source(NodeId::kRandomId);
//  NonEmptyString content(RandomAlphaNumericString(256));
//  asymm::Signature signature;
//  nfs::Message message(nfs::ActionType::kPut,
//                       nfs::PersonaType::kDataHolder,
//                       nfs::PersonaType::kPmidAccountHolder,
//                       0,
//                       destination,
//                       source,
//                       content,
//                       signature);

//  std::string retrieved;
//  data_holder_->HandlePutMessage(message,
//                                 [&](const std::string&) {});
//  retrieved = data_holder_->permenent_data_store.Get(message);
//  EXPECT_EQ(message.content(), retrieved);
//}

//TEST(DataHolderTest, BEH_HandleGetMessage) {
//  NodeId destination(this->routing_->kNodeId()), source(NodeId::kRandomId);
//  NonEmptyString content(RandomAlphaNumericString(256));
//  asymm::Signature signature;
//  nfs::Message message(nfs::ActionType::kPut,
//                       nfs::PersonaType::kDataHolder,
//                       nfs::PersonaType::kPmidAccountHolder,
//                       0,
//                       destination,
//                       source,
//                       content,
//                       signature);
//  std::string retrieved;
//  this->data_holder_->handleGetMessage(message,
//                                       [&](const std::string& data) {
//                                         retrieved = data;
//                                       });
//  EXPECT_TRUE(retrieved.empty());
//  data_holder_->HandlePutMessage(message,
//                                 [&](const std::string&) {});
//  this->data_holder_->handleGetMessage(message,
//                                       [&](const std::string& data) {
//                                         retrieved = data;
//                                       });
//  EXPECT_TRUE(!retrieved.empty());
//}

//TEST(DataHolderTest, BEH_HandlePostMessage) {
//}

//TEST(DataHolderTest, BEH_HandleDeleteMessage) {
//  NodeId destination(this->routing_->kNodeId()), source(NodeId::kRandomId);
//  NonEmptyString content(RandomAlphaNumericString(256));
//  asymm::Signature signature;
//  nfs::Message message(nfs::ActionType::kPut,
//                       nfs::PersonaType::kDataHolder,
//                       nfs::PersonaType::kPmidAccountHolder,
//                       0,
//                       destination,
//                       source,
//                       content,
//                       signature);
//  std::string retrieved;
//  data_holder_->HandlePutMessage(message,
//                                 [&](const std::string&) {});
//  this->data_holder_->handleGetMessage(message,
//                                       [&](const std::string& data) {
//                                         retrieved = data;
//                                       });
//  EXPECT_TRUE(!retrieved.empty());
//  this->data_holder_->handleDeleteMessage(message,
//                                       [&](const std::string& data) {
//                                         retrieved = data;
//                                       });
//  this->data_holder_->handleGetMessage(message,
//                                       [&](const std::string& data) {
//                                         retrieved = data;
//                                       });
//  EXPECT_TRUE(retrieved.empty());
//}

//}  // namespace test

//}  // namespace vault

//}  // namespace maidsafe

