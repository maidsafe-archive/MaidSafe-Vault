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

#include "maidsafe/vault/group_db.h"

namespace maidsafe {

namespace vault {

template <>
GroupDb<PmidManager>::GroupMap::iterator GroupDb<PmidManager>::FindOrCreateGroup(
    const GroupName& group_name) {
  try {
    return FindGroup(group_name);
  } catch (const vault_error& error) {
    LOG(kInfo) << "Account doesn't exist for group "
               << HexSubstr(group_name->string()) << ", error : " << error.what()
               << ". -- Creating Account --";
    return AddGroupToMap(group_name, Metadata(group_name));
  }
}

template <>
void GroupDb<PmidManager>::Commit(
    const Key& key,
    std::function<detail::DbAction(Metadata& metadata, std::unique_ptr<Value>& value)> functor) {
  LOG(kVerbose) << "GroupDb<PmidManager>::Commit update metadata and value for account "
                << HexSubstr(key.group_name()->string());
  assert(functor);
  std::lock_guard<std::mutex> lock(mutex_);
  try {
    const auto it(FindGroup(key.group_name()));
    on_scope_exit update_group([it, this]() { UpdateGroup(it); });
    std::unique_ptr<Value> value;
    try {
      value.reset(new Value(Get(key, it->second.first)));
    } catch (const common_error& error) {
      if (error.code().value() != static_cast<int>(CommonErrors::no_such_element)) {
        LOG(kError) << "error when trying to get value, error code is " << error.what();
        throw error;  // throw only for db errors
      }
    }

    if (detail::DbAction::kPut == functor(it->second.second, value)) {
      LOG(kInfo) << "putting into group_db";
      Put(std::make_pair(key, std::move(*value)), it->second.first);
    } else {
      assert(value);
      LOG(kInfo) << "deleting from group_db";
      Delete(key, it->second.first);
    }
  } catch (const maidsafe_error& error) {
    LOG(kInfo) << "Account doesn't exist for group "
               << HexSubstr(key.group_name()->string()) << ", error : " << error.what()
               << ". -- Creating Account --";
    if (error.code() == VaultErrors::no_such_account) {
      Metadata temp(key.group_name());
      std::unique_ptr<Value> value;
      functor(temp, value);
      AddGroupToMap(key.group_name(), temp);
    } else {
      throw error;
    }
  }
}

// template <>
// GroupDb<PmidManager>::GroupMap::iterator GroupDb<PmidManager>::FindOrCreateGroup(
//     const GroupName& group_name) {
//   try {
//     return FindGroup(group_name);
//   } catch (const vault_error& error) {
//     LOG(kInfo) << "Account doesn't exist for group "
//                << HexSubstr(group_name->string()) << ", error : " << error.what()
//                << ". -- Creating Account --";
//     return AddGroupToMap(group_name, Metadata(group_name));
//   }
// }

// Deletes group if no further entry left in group
template <>
void GroupDb<PmidManager>::UpdateGroup(typename GroupMap::iterator it) {
  if (it->second.second.GroupStatus() == detail::GroupDbMetaDataStatus::kGroupEmpty) {
    LOG(kInfo) << "Account empty for group " << HexSubstr(it->first->string())
               << ". -- Deleteing Account --";
    DeleteGroupEntries(it);
  }
}

}  // namespace vault

}  // namespace maidsafe
