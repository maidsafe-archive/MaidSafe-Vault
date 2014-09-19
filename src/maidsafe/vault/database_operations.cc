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

#include <cstdint>
#include <string>

#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"

namespace maidsafe {

namespace vault {

VaultDataBase::VaultDataBase(const boost::filesystem::path& db_path)
  : data_base_(), seeking_statement_() {
  data_base_.reset(new sqlite::Database(db_path,
                                        sqlite::Mode::kReadWriteCreate));
  std::string query(
      "CREATE TABLE IF NOT EXISTS KeyValuePairs ("
      "KEY TEXT  PRIMARY KEY NOT NULL, VALUE TEXT NOT NULL);");
  sqlite::Tranasction transaction{*data_base_};
  sqlite::Statement statement{*data_base_, query};
  statement.Step();
  transaction.Commit();
}

void VaultDataBase::Put(const KEY& key, const VALUE& value) {
  if (!data_base_)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::db_not_presented));

  sqlite::Tranasction transaction{*data_base_};
  std::string query(
      "INSERT OR REPLACE INTO KeyValuePairs (KEY, VALUE) VALUES (?, ?)");
  sqlite::Statement statement{*data_base_, query};
  statement.BindText(1, key);
  statement.BindText(2, value);
  statement.Step();
  transaction.Commit();
}

void VaultDataBase::Get(const KEY& key, VALUE& value) {
  if (!data_base_)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::db_not_presented));

  std::string query(
      "SELECT VALUE FROM KeyValuePairs WHERE KEY=?");
  sqlite::Statement statement{*data_base_, query};
  statement.BindText(1, key);
  if (statement.Step() == sqlite::StepResult::kSqliteRow)
    value = statement.ColumnText(0);
}

void VaultDataBase::Delete(const KEY& key) {
  if (!data_base_)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::db_not_presented));

  sqlite::Tranasction transaction{*data_base_};
  std::string query(
      "DELETE FROM KeyValuePairs WHERE KEY=?");
  sqlite::Statement statement{*data_base_, query};
  statement.BindText(1, key);
  statement.Step();
  transaction.Commit();
}

bool VaultDataBase::SeekNext(std::pair<KEY, VALUE>& result) {
  if (!data_base_)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::db_not_presented));

  if (!seeking_statement_) {
    std::string query("SELECT * from KeyValuePairs");
    seeking_statement_.reset(new sqlite::Statement(*data_base_, query));
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

}  // namespace vault

}  // namespace maidsafe

