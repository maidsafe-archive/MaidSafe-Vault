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

#include "maidsafe/vault/maid_manager/action_put.h"
#include "maidsafe/vault/maid_manager/action_delete.h"
#include "maidsafe/vault/action_create_remove_account.h"
#include "maidsafe/vault/maid_manager/maid_manager.h"
#include "maidsafe/vault/maid_manager/value.h"

namespace maidsafe {

namespace vault {

namespace test {


TEST(MaidManagerActionsTest, BEH_Construct) {
  {
    // put
    decltype(ActionMaidManagerPut::kSize) size(1);
    ActionMaidManagerPut action(size);
    ASSERT_EQ(nfs::MessageAction::kPutRequest, action.kActionId);
    ASSERT_EQ(size, action.kSize);

    ASSERT_NO_THROW(ActionMaidManagerPut action_(action));
    ASSERT_NO_THROW(ActionMaidManagerPut(std::move(action)));
  }
  {
    // delete
    decltype(ActionMaidManagerDelete::kMessageId) message_id(RandomInt32());
    ActionMaidManagerDelete action(message_id);
    ASSERT_EQ(nfs::MessageAction::kDeleteRequest, action.kActionId);
    ASSERT_EQ(message_id, action.kMessageId);

    ASSERT_NO_THROW(ActionMaidManagerDelete action_(action));
    ASSERT_NO_THROW(ActionMaidManagerDelete(std::move(action)));
  }
  {
    // create/remove account true
    decltype(ActionCreateRemoveAccount<true>::kMessageId) message_id(RandomInt32());
    ActionCreateRemoveAccount<true> action(message_id);
    ASSERT_EQ(nfs::MessageAction::kRemoveAccountRequest, action.kActionId);
    ASSERT_EQ(message_id, action.kMessageId);

    ASSERT_NO_THROW(ActionCreateRemoveAccount<true> action_(action));
    ASSERT_NO_THROW(ActionCreateRemoveAccount<true>(std::move(action)));
  }
  {
    // create/remove account false
    decltype(ActionCreateRemoveAccount<false>::kMessageId) message_id(RandomInt32());
    ActionCreateRemoveAccount<false> action(message_id);
    ASSERT_EQ(nfs::MessageAction::kCreateAccountRequest, action.kActionId);
    ASSERT_EQ(message_id, action.kMessageId);

    ASSERT_NO_THROW(ActionCreateRemoveAccount<false> action_(action));
    ASSERT_NO_THROW(ActionCreateRemoveAccount<false>(std::move(action)));
  }
}

TEST(MaidManagerActionsTest, BEH_EqualityInequality) {
  {
    // put
    decltype(ActionMaidManagerPut::kSize) size1(1), size2(2);
    ActionMaidManagerPut action1(size1), action2(size2), action3(size1);

    ASSERT_EQ(nfs::MessageAction::kPutRequest, action1.kActionId);
    ASSERT_EQ(size1, action1.kSize);
    ASSERT_EQ(nfs::MessageAction::kPutRequest, action2.kActionId);
    ASSERT_EQ(size2, action2.kSize);
    ASSERT_EQ(nfs::MessageAction::kPutRequest, action3.kActionId);
    ASSERT_EQ(size1, action3.kSize);

    ASSERT_FALSE(action1 == action2);
    ASSERT_TRUE(action1 == action3);
    ASSERT_TRUE(action1 != action2);
    ASSERT_FALSE(action1 != action3);
  }
  {
    // delete
    decltype(ActionMaidManagerDelete::kMessageId) message1_id(RandomInt32()),
                                                  message2_id(RandomInt32());
    ActionMaidManagerDelete action1(message1_id), action2(message2_id),
                            action3(message1_id);

    ASSERT_EQ(nfs::MessageAction::kDeleteRequest, action1.kActionId);
    ASSERT_EQ(message1_id, action1.kMessageId);
    ASSERT_EQ(nfs::MessageAction::kDeleteRequest, action2.kActionId);
    ASSERT_EQ(message2_id, action2.kMessageId);
    ASSERT_EQ(nfs::MessageAction::kDeleteRequest, action3.kActionId);
    ASSERT_EQ(message1_id, action3.kMessageId);

    ASSERT_FALSE(action1 == action2);
    ASSERT_TRUE(action1 == action3);
    ASSERT_TRUE(action1 != action2);
    ASSERT_FALSE(action1 != action3);
  }
  {
    // create/remove account true
    decltype(ActionCreateRemoveAccount<true>::kMessageId) message1_id(RandomInt32()),
                                                          message2_id(RandomInt32());
    ActionCreateRemoveAccount<true> action1(message1_id), action2(message2_id),
                                    action3(message1_id);

    ASSERT_EQ(nfs::MessageAction::kRemoveAccountRequest, action1.kActionId);
    ASSERT_EQ(message1_id, action1.kMessageId);
    ASSERT_EQ(nfs::MessageAction::kRemoveAccountRequest, action2.kActionId);
    ASSERT_EQ(message2_id, action2.kMessageId);
    ASSERT_EQ(nfs::MessageAction::kRemoveAccountRequest, action3.kActionId);
    ASSERT_EQ(message1_id, action3.kMessageId);

    ASSERT_FALSE(action1 == action2);
    ASSERT_TRUE(action1 == action3);
    ASSERT_TRUE(action1 != action2);
    ASSERT_FALSE(action1 != action3);
  }
  {
    // create/remove account false
    decltype(ActionCreateRemoveAccount<false>::kMessageId) message1_id(RandomInt32()),
                                                           message2_id(RandomInt32());
    ActionCreateRemoveAccount<false> action1(message1_id), action2(message2_id),
                                     action3(message1_id);

    ASSERT_EQ(nfs::MessageAction::kCreateAccountRequest, action1.kActionId);
    ASSERT_EQ(message1_id, action1.kMessageId);
    ASSERT_EQ(nfs::MessageAction::kCreateAccountRequest, action2.kActionId);
    ASSERT_EQ(message2_id, action2.kMessageId);
    ASSERT_EQ(nfs::MessageAction::kCreateAccountRequest, action3.kActionId);
    ASSERT_EQ(message1_id, action3.kMessageId);

    ASSERT_FALSE(action1 == action2);
    ASSERT_TRUE(action1 == action3);
    ASSERT_TRUE(action1 != action2);
    ASSERT_FALSE(action1 != action3);
  }
}

TEST(MaidManagerActionsTest, BEH_SerialiseParse) {
  {
    // put
    decltype(ActionMaidManagerPut::kSize) size(1);
    ActionMaidManagerPut action(size);
    std::string serialised_action;
    ASSERT_THROW(ActionMaidManagerPut action0(serialised_action), maidsafe_error);
    ASSERT_NO_THROW(serialised_action = action.Serialise());
    ActionMaidManagerPut action0(serialised_action);
    ASSERT_EQ(nfs::MessageAction::kPutRequest, action0.kActionId);
    ASSERT_EQ(size, action0.kSize);
  }
  {
    // delete
    decltype(ActionMaidManagerDelete::kMessageId) message_id(RandomInt32());
    ActionMaidManagerDelete action(message_id);
    std::string serialised_action;
    ASSERT_THROW(ActionMaidManagerDelete action0(serialised_action), maidsafe_error);
    ASSERT_NO_THROW(serialised_action = action.Serialise());
    ActionMaidManagerDelete action0(serialised_action);
    ASSERT_EQ(nfs::MessageAction::kDeleteRequest, action0.kActionId);
    ASSERT_EQ(message_id, action0.kMessageId);
  }
  {
    // create/remove account true
    decltype(ActionCreateRemoveAccount<true>::kMessageId) message_id(RandomInt32());
    ActionCreateRemoveAccount<true> action(message_id);
    std::string serialised_action;
    ASSERT_THROW(ActionCreateRemoveAccount<true> action0(serialised_action), maidsafe_error);
    ASSERT_NO_THROW(serialised_action = action.Serialise());
    ActionCreateRemoveAccount<true> action0(serialised_action);
    ASSERT_EQ(nfs::MessageAction::kRemoveAccountRequest, action0.kActionId);
    ASSERT_EQ(message_id, action0.kMessageId);
  }
  {
    // create/remove account false
    decltype(ActionCreateRemoveAccount<false>::kMessageId) message_id(RandomInt32());
    ActionCreateRemoveAccount<false> action(message_id);
    std::string serialised_action;
    ASSERT_THROW(ActionCreateRemoveAccount<false> action0(serialised_action), maidsafe_error);
    ASSERT_NO_THROW(serialised_action = action.Serialise());
    ActionCreateRemoveAccount<false> action0(serialised_action);
    ASSERT_EQ(nfs::MessageAction::kCreateAccountRequest, action0.kActionId);
    ASSERT_EQ(message_id, action0.kMessageId);
  }
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
