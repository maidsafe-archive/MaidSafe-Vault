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

#include "maidsafe/vault/database_operations.h"

#include <utility>
#include <cstdint>
#include <string>

#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"

namespace maidsafe {

namespace vault {

VaultDatabase::VaultDatabase(const boost::filesystem::path& db_path)
  : database_(), seeking_statement_(), write_operations_(0) {
  database_.reset(new sqlite::Database(db_path,
                                        sqlite::Mode::kReadWriteCreate));
  std::string query(
      "CREATE TABLE IF NOT EXISTS KeyValuePairs ("
      "KEY TEXT  PRIMARY KEY NOT NULL, VALUE TEXT NOT NULL);");
  sqlite::Transaction transaction{*database_};
  sqlite::Statement statement{*database_, query};
  statement.Step();
  transaction.Commit();
}

void VaultDatabase::Put(const KEY& key, const VALUE& value) {
  if (!database_)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::db_not_present));
  CheckPoint();

  sqlite::Transaction transaction{*database_};
  std::string query(
      "INSERT OR REPLACE INTO KeyValuePairs (KEY, VALUE) VALUES (?, ?)");
  sqlite::Statement statement{*database_, query};
  statement.BindText(1, key);
  statement.BindText(2, value);
  statement.Step();
  transaction.Commit();
}

void VaultDatabase::Get(const KEY& key, VALUE& value) {
  if (!database_)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::db_not_present));

  std::string query(
      "SELECT VALUE FROM KeyValuePairs WHERE KEY=?");
  sqlite::Statement statement{*database_, query};
  statement.BindText(1, key);
  if (statement.Step() == sqlite::StepResult::kSqliteRow)
    value = statement.ColumnText(0);
}

void VaultDatabase::Delete(const KEY& key) {
  if (!database_)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::db_not_present));
  CheckPoint();

  sqlite::Transaction transaction{*database_};
  std::string query(
      "DELETE FROM KeyValuePairs WHERE KEY=?");
  sqlite::Statement statement{*database_, query};
  statement.BindText(1, key);
  statement.Step();
  transaction.Commit();
}

bool VaultDatabase::SeekNext(std::pair<KEY, VALUE>& result) {
  if (!database_)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::db_not_present));

  if (!seeking_statement_) {
    std::string query("SELECT * from KeyValuePairs");
    seeking_statement_.reset(new sqlite::Statement(*database_, query));
  }
  if (seeking_statement_->Step() == sqlite::StepResult::kSqliteRow) {
    result = std::make_pair(seeking_statement_->ColumnText(0),
                            seeking_statement_->ColumnText(1));
    return true;
  } else {
    seeking_statement_.reset();
    return false;
  }
}

void VaultDatabase::CheckPoint() {
  if (++write_operations_ > 1000) {
    database_->CheckPoint();
    write_operations_ = 0;
  }
}

}  // namespace vault

}  // namespace maidsafe

