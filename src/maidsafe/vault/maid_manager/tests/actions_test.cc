/*  Copyright 2012 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.maidsafe.net/licenses

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#include <vector>

#include "maidsafe/common/test.h"

#include "maidsafe/nfs/types.h"
#include "maidsafe/vault/maid_manager/action_put.h"
#include "maidsafe/vault/maid_manager/action_delete.h"
#include "maidsafe/vault/maid_manager/maid_manager.h"
#include "maidsafe/vault/maid_manager/value.h"

namespace maidsafe {

namespace vault {

namespace test {


TEST(MaidManagerActionsTest, BEH_Construct) {
  decltype(ActionMaidManagerPut::kSize) size(1);
  ActionMaidManagerPut action_put(size);
  ASSERT_EQ(nfs::MessageAction::kPutRequest, action_put.kActionId);
  ASSERT_EQ(size, action_put.kSize);

  ASSERT_NO_THROW(ActionMaidManagerPut action_put0(action_put));
//  ASSERT_NO_THROW(ActionMaidManagerPut(action_put));

  decltype(ActionMaidManagerDelete::kMessageId) message_id(RandomInt32());
  ActionMaidManagerDelete action_delete(message_id);
  ASSERT_EQ(nfs::MessageAction::kDeleteRequest, action_delete.kActionId);
  ASSERT_EQ(message_id, action_delete.kMessageId);

  ASSERT_NO_THROW(ActionMaidManagerDelete action_delete0(action_delete));
//  ASSERT_NO_THROW(ActionMaidManagerDelete(action_delete));
}

TEST(MaidManagerActionsTest, BEH_EqualityInequality) {
  decltype(ActionMaidManagerPut::kSize) size1(1), size2(2);
  ActionMaidManagerPut action1_put(size1), action2_put(size2), action3_put(size1);
  
  ASSERT_EQ(nfs::MessageAction::kPutRequest, action1_put.kActionId);
  ASSERT_EQ(size1, action1_put.kSize);
  ASSERT_EQ(nfs::MessageAction::kPutRequest, action2_put.kActionId);
  ASSERT_EQ(size2, action2_put.kSize);
  ASSERT_EQ(nfs::MessageAction::kPutRequest, action3_put.kActionId);
  ASSERT_EQ(size1, action3_put.kSize);

  ASSERT_FALSE(action1_put == action2_put);
  ASSERT_TRUE(action1_put == action3_put);
  ASSERT_TRUE(action1_put != action2_put);
  ASSERT_FALSE(action1_put != action3_put);
  
  decltype(ActionMaidManagerDelete::kMessageId) message1_id(RandomInt32()),
                                                message2_id(RandomInt32());
  ActionMaidManagerDelete action1_delete(message1_id), action2_delete(message2_id),
                          action3_delete(message1_id);

  ASSERT_EQ(nfs::MessageAction::kDeleteRequest, action1_delete.kActionId);
  ASSERT_EQ(message1_id, action1_delete.kMessageId);
  ASSERT_EQ(nfs::MessageAction::kDeleteRequest, action2_delete.kActionId);
  ASSERT_EQ(message2_id, action2_delete.kMessageId);
  ASSERT_EQ(nfs::MessageAction::kDeleteRequest, action3_delete.kActionId);
  ASSERT_EQ(message1_id, action3_delete.kMessageId);

  ASSERT_FALSE(action1_delete == action2_delete);
  ASSERT_TRUE(action1_delete == action3_delete);
  ASSERT_TRUE(action1_delete != action2_delete);
  ASSERT_FALSE(action1_delete != action3_delete);
}

TEST(MaidManagerActionsTest, BEH_SerialiseParse) {
  decltype(ActionMaidManagerPut::kSize) size(1);
  ActionMaidManagerPut action_put(size); 
  std::string serialised_action_put;
  ASSERT_THROW(ActionMaidManagerPut action0_put(serialised_action_put), maidsafe_error);
  ASSERT_NO_THROW(serialised_action_put = action_put.Serialise());
  ActionMaidManagerPut action0_put(serialised_action_put);
  ASSERT_EQ(nfs::MessageAction::kPutRequest, action0_put.kActionId);
  ASSERT_EQ(size, action0_put.kSize);

  decltype(ActionMaidManagerDelete::kMessageId) message_id(RandomInt32());
  ActionMaidManagerDelete action_delete(message_id);
  std::string serialised_action_delete;
  ASSERT_THROW(ActionMaidManagerDelete action0_delete(serialised_action_delete), maidsafe_error);
  ASSERT_NO_THROW(serialised_action_delete = action_delete.Serialise());
  ActionMaidManagerDelete action0_delete(serialised_action_delete);
  ASSERT_EQ(nfs::MessageAction::kDeleteRequest, action0_delete.kActionId);
  ASSERT_EQ(message_id, action0_delete.kMessageId);
}

TEST(MaidManagerActionsTest, BEH_PutOperator) {
  MaidManager::Value value;
  ASSERT_EQ(0, value.data_stored);
  ASSERT_EQ(std::numeric_limits<uint64_t>().max(), value.space_available);

  decltype(ActionMaidManagerPut::kSize) size(1);
  ActionMaidManagerPut action_put(size); 
  
  ASSERT_NO_THROW(action_put(value));

  ASSERT_EQ(size, value.data_stored);
  ASSERT_EQ(std::numeric_limits<uint64_t>().max() - size, value.space_available);
}


}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
