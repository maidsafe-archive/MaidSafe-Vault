/* Copyright 2013 MaidSafe.net limited

This MaidSafe Software is licensed under the MaidSafe.net Commercial License, version 1.0 or later,
and The General Public License (GPL), version 3. By contributing code to this project You agree to
the terms laid out in the MaidSafe Contributor Agreement, version 1.0, found in the root directory
of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available at:

http://www.novinet.com/license

Unless required by applicable law or agreed to in writing, software distributed under the License is
distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.
*/

#ifndef MAIDSAFE_VAULT_PMID_MANAGER_PMID_MANAGER_MERGE_POLICY_H_
#define MAIDSAFE_VAULT_PMID_MANAGER_PMID_MANAGER_MERGE_POLICY_H_

#include <map>
#include <set>
#include <utility>
#include <vector>

#include "maidsafe/common/tagged_value.h"
#include "maidsafe/common/types.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/vault/account_db.h"
#include "maidsafe/vault/unresolved_element.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.h"


namespace maidsafe {
namespace vault {

class PmidAccountMergePolicy {
 public:
  typedef PmidAccountUnresolvedEntry UnresolvedEntry;
  typedef PmidAccountResolvedEntry ResolvedEntry;
  typedef AccountDb Database;
  explicit PmidAccountMergePolicy(AccountDb* account_db);
  PmidAccountMergePolicy(PmidAccountMergePolicy&& other);
  PmidAccountMergePolicy& operator=(PmidAccountMergePolicy&& other);
  // This flags a "Put" entry in 'unresolved_data_' as not to be added to the db.
  template<typename Data>
  int32_t AllowDelete(const typename Data::name_type& name);

 protected:
  void Merge(const UnresolvedEntry& unresolved_entry);

  std::vector<UnresolvedEntry> unresolved_data_;
  AccountDb* account_db_;

 private:
  typedef TaggedValue<int32_t, struct SizeTag> Size;
  typedef TaggedValue<int32_t, struct CountTag> Count;

  PmidAccountMergePolicy(const PmidAccountMergePolicy&);
  PmidAccountMergePolicy& operator=(const PmidAccountMergePolicy&);

  void MergePut(const DataNameVariant& data_name,
                UnresolvedEntry::Value size,
                const NonEmptyString& serialised_db_value);
  void MergeDelete(const DataNameVariant& data_name, const NonEmptyString& serialised_db_value);
  NonEmptyString SerialiseDbValue(Size size) const;
  Size ParseDbValue(NonEmptyString serialised_db_value) const;
  NonEmptyString GetFromDb(const DataNameVariant& data_name);
};

template<typename Data>
int32_t PmidAccountMergePolicy::AllowDelete(const typename Data::name_type& name) {
  auto serialised_db_value(GetFromDb(name));
  Size size(0);
  Count current_count(0);
  if (serialised_db_value.IsInitialised())
    return ParseDbValue(serialised_db_value);

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
          if (size != 0 && (*itr).messages_contents.front().value)
            size.data = *(*itr).messages_contents.front().value;
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

#endif  // MAIDSAFE_VAULT_PMID_MANAGER_PMID_MANAGER_MERGE_POLICY_H_
