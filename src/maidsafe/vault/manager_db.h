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

#ifndef MAIDSAFE_VAULT_MANAGER_DB_H_
#define MAIDSAFE_VAULT_MANAGER_DB_H_

#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "leveldb/db.h"


namespace maidsafe {

namespace vault {

template<typename PersonaType>
class ManagerDb {
 public:
  typedef std::pair<typename PersonaType::DbKey, typename PersonaType::DbValue> KvPair;
//  explicit ManagerDb(const boost::filesystem::path& path);
  ManagerDb();
  ~ManagerDb();

  void Put(const KvPair& key_value_pair);
  void Delete(const typename PersonaType::DbKey& key);
  typename PersonaType::DbValue Get(const typename PersonaType::DbKey& key);
  std::vector<typename PersonaType::DbKey> GetKeys();

 private:
  ManagerDb(const ManagerDb&);
  ManagerDb& operator=(const ManagerDb&);
  ManagerDb(ManagerDb&&);
  ManagerDb& operator=(ManagerDb&&);

  const boost::filesystem::path kDbPath_;
  mutable std::mutex mutex_;
  std::unique_ptr<leveldb::DB> leveldb_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/manager_db-inl.h"

#endif  // MAIDSAFE_VAULT_MANAGER_DB_H_
