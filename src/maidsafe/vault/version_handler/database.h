/*  Copyright 2015 MaidSafe.net limited

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

#ifndef MAIDSAFE_VAULT_VERSION_HANDLER_DATABASE_H_
#define MAIDSAFE_VAULT_VERSION_HANDLER_DATABASE_H_

#include <string>
#include <utility>

#include "maidsafe/common/sqlite3_wrapper.h"

namespace maidsafe {

namespace vault {

class VersionHandlerDatabase {
  typedef std::string VALUE;
 public:
  typedef std::string KEY;
  explicit VersionHandlerDatabase(const boost::filesystem::path& db_path);
  ~VersionHandlerDatabase();

  void Put(const KEY& key, const VALUE& value);
  void Get(const KEY& key, VALUE& value);
  void Delete(const KEY& key);
  bool SeekNext(std::pair<KEY, VALUE>& result);

 private:
  void CheckPoint();

  std::unique_ptr<sqlite::Database> database_;
  std::unique_ptr<sqlite::Statement> seeking_statement_;
  const boost::filesystem::path kDbPath_;
  int write_operations_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VERSION_HANDLER_DATABASE_H_
