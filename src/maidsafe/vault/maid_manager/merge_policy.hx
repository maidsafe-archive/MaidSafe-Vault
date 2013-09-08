/*  Copyright 2013 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.novinet.com/license

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_MAID_MANAGER_MERGE_POLICY_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_MAID_MANAGER_MERGE_POLICY_H_

#include <map>
#include <set>
#include <utility>
#include <vector>

#include "maidsafe/common/tagged_value.h"
#include "maidsafe/common/types.h"
#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/unresolved_action.h"
#include "maidsafe/vault/maid_manager/maid_manager.h"

namespace maidsafe {

namespace vault {

class AccountDb;

class MaidManagerMergePolicy {
 public:
  typedef MaidManagerUnresolvedAction UnresolvedAction;
  typedef MaidManagerResolvedAction ResolvedAction;
  typedef AccountDb Database;
  explicit MaidManagerMergePolicy(AccountDb* account_db);
  MaidManagerMergePolicy(MaidManagerMergePolicy&& other);
  MaidManagerMergePolicy& operator=(MaidManagerMergePolicy&& other);
  // This flags a "Put" action in 'unresolved_data_' as not to be added to the db.
  template<typename Data>
  int32_t AllowDelete(const typename Data::Name& name);

 protected:
  void Merge(const UnresolvedAction& unresolved_action);

  std::vector<UnresolvedAction> unresolved_data_;
  AccountDb* account_db_;

 private:
  typedef TaggedValue<int32_t, struct AverageCostTag> AverageCost;
  typedef TaggedValue<int32_t, struct CountTag> Count;
  typedef std::pair<AverageCost, Count> DbValue;

  MaidManagerMergePolicy(const MaidManagerMergePolicy&);
  MaidManagerMergePolicy& operator=(const MaidManagerMergePolicy&);

  UnresolvedAction::Value MergedCost(const UnresolvedAction& unresolved_action) const;
  void MergePut(const DataNameVariant& data_name,
                UnresolvedAction::Value cost,
                const NonEmptyString& serialised_db_value);
  void MergeDelete(const DataNameVariant& data_name, const NonEmptyString& serialised_db_value);
  NonEmptyString SerialiseDbValue(DbValue db_value) const;
  DbValue ParseDbValue(NonEmptyString serialised_db_value) const;
  NonEmptyString GetFromDb(const DataNameVariant& data_name);
};

template<typename Data>
int32_t MaidManagerMergePolicy::AllowDelete(const typename Data::Name& name) {
  auto serialised_db_value(GetFromDb(name));
  Count current_count(0);
  AverageCost size(0);
  if (serialised_db_value.IsInitialised()) {
    auto current_values(ParseDbValue(serialised_db_value));
    assert(current_values.second.data > 0);
    current_count = current_values.second;
    size.data = current_values.first;
  }

  DataNameVariant name_as_variant(name);
  auto itr(std::begin(unresolved_data_));
  auto last_put_still_to_be_added_to_db(std::end(unresolved_data_));
  int32_t pending_puts(0), pending_deletes(0);

  while (itr != std::end(unresolved_data_)) {
    if ((*itr).key.first == name_as_variant) {
      if ((*itr).key.second == nfs::MessageAction::kPut) {
        if ((*itr).dont_add_to_db) {
          // A delete request must have been applied for this to be true, but it will (correctly)
          // silently fail when it comes to merging since this put request will not have been
          // added to the account_db.
          --pending_deletes;
        } else {
          ++pending_puts;
          last_put_still_to_be_added_to_db = itr;
          if (size != 0 && (*itr).messages_contents.front().value) {
              // TODO(dirvine) we should average these results rather than taking the first
            size.data = *(*itr).messages_contents.front().value;
          }
        }
      } else {
        assert((*itr).key.second == nfs::MessageAction::kDelete);
        ++pending_deletes;
      }
    }
    ++itr;
  }

  if (current_count <= pending_deletes &&
      last_put_still_to_be_added_to_db != std::end(unresolved_data_)) {
    (*last_put_still_to_be_added_to_db).dont_add_to_db = true;
  }

  if (current_count + pending_puts <= pending_deletes)
    size.data = 0;
  return size.data;
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_MAID_MANAGER_MERGE_POLICY_H_
