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

#ifndef MAIDSAFE_VAULT_DATA_MANAGER_ACTION_SET_PMID_ONLINE_H_
#define MAIDSAFE_VAULT_DATA_MANAGER_ACTION_SET_PMID_ONLINE_H_

#include <cstdint>
#include <string>

#include "boost/optional/optional.hpp"

#include "maidsafe/nfs/types.h"
#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

class DataManagerValue;

struct ActionDataManagerSetPmidOnline {
 public:
  explicit ActionDataManagerSetPmidOnline(const PmidName& pmid_name);
  explicit ActionDataManagerSetPmidOnline(const std::string& serialised_action);
  ActionDataManagerSetPmidOnline(const ActionDataManagerSetPmidOnline& other);
  ActionDataManagerSetPmidOnline(ActionDataManagerSetPmidOnline&& other);
  std::string Serialise() const;

  void operator()(boost::optional<DataManagerValue>& value);

  static const nfs::MessageAction kActionId = nfs::MessageAction::kSetPmidOnline;
  const PmidName kPmidName;

 private:
  ActionDataManagerSetPmidOnline();
  ActionDataManagerSetPmidOnline& operator=(ActionDataManagerSetPmidOnline other);
};

bool operator==(const ActionDataManagerSetPmidOnline& lhs,
                const ActionDataManagerSetPmidOnline& rhs);

bool operator!=(const ActionDataManagerSetPmidOnline& lhs,
                const ActionDataManagerSetPmidOnline& rhs);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_ACTION_SET_PMID_ONLINE_H_
