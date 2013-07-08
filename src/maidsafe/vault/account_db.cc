/* Copyright 2012 MaidSafe.net limited

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

#include "maidsafe/vault/account_db.h"

#include "maidsafe/common/log.h"
#include "maidsafe/data_types/data_type_values.h"


namespace maidsafe {

namespace vault {

AccountDb::AccountDb(Db& db) : db_(db), account_id_(db.RegisterAccount()) {}

AccountDb::~AccountDb() {
  db_.UnRegisterAccount(account_id_);
}

void AccountDb::Put(const Db::KvPair& key_value_pair) {
  db_.Put(account_id_, key_value_pair);
}

void AccountDb::Delete(const Db::KvPair::first_type& key) {
  db_.Delete(account_id_, key);
}

NonEmptyString AccountDb::Get(const Db::KvPair::first_type& key) {
  return db_.Get(account_id_, key);
}

std::vector<Db::KvPair> AccountDb::Get() {
  return db_.Get(account_id_);
}

}  // namespace vault
}  // namespace maidsafe
