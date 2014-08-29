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
  LOG(kVerbose) << "GroupDb<PmidManager>::FindOrCreateGroup " << HexSubstr(group_name->string());
  try {
    return FindGroup(group_name);
  }
  catch (const maidsafe_error& error) {
    if (error.code() == make_error_code(VaultErrors::no_such_account))
      LOG(kInfo) << "Account doesn't exist for group " << HexSubstr(group_name->string())
                 << ". -- Creating Account --";
    else
      LOG(kError) << "GroupDb<PmidManager>::FindOrCreateGroup encountered unknown error "
                  << boost::diagnostic_information(error);
    return AddGroupToMap(group_name, Metadata(group_name));
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
  LOG(kVerbose) << "GroupDb<PmidManager>::UpdateGroup " << HexSubstr(it->first->string());
  if (it->second.second.GroupStatus() == detail::GroupDbMetaDataStatus::kGroupEmpty) {
    LOG(kInfo) << "Account empty for group " << HexSubstr(it->first->string())
               << ". -- Deleteing Account --";
    DeleteGroupEntries(it);
  }
}

}  // namespace vault

}  // namespace maidsafe
