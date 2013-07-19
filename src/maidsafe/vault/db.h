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

#ifndef MAIDSAFE_VAULT_DB_H_
#define MAIDSAFE_VAULT_DB_H_

#include <cstdint>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "boost/filesystem/path.hpp"
#include "leveldb/db.h"

#include "maidsafe/common/types.h"

#include "maidsafe/vault/group_key.h"


namespace maidsafe {

namespace vault {

class AccountDb;

class Db {
 public:
  typedef std::pair<NonEmptyString, NonEmptyString> KvPair;
  typedef uint32_t AccountId;

//  explicit Db(const boost::filesystem::path& path);
  Db();
  ~Db();
  friend class AccountDb;

 private:
  Db(const Db&);
  Db& operator=(const Db&);
  Db(Db&&);
  Db& operator=(Db&&);

  AccountId RegisterAccount();
  void UnRegisterAccount(const AccountId& account_id);

  void Put(const AccountId& account_id, const KvPair& key_value_pair);
  void Delete(const AccountId& account_id, const KvPair::first_type& key);
  NonEmptyString Get(const AccountId& account_id, const KvPair::first_type& key);
  std::vector<KvPair> Get(const AccountId& account_id);

  std::string SerialiseKey(const AccountId& account_id, const KvPair::first_type& key) const;
  std::pair<AccountId, KvPair::first_type> ParseKey(const std::string& serialised_key) const;

  static const int kPrefixWidth_;
  const boost::filesystem::path kDbPath_;
  std::mutex mutex_;
  std::unique_ptr<leveldb::DB> leveldb_;
  std::set<AccountId> account_ids_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DB_H_
