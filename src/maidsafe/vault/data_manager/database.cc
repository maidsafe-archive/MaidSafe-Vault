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

#include <string>

#include "boost/filesystem.hpp"

#include "maidsafe/vault/data_manager/database.h"

namespace maidsafe {

namespace vault {

DataManagerDatabase::DataManagerDatabase(const boost::filesystem::path& db_path)
    : database_(), kDbPath_(db_path), write_operations_(0) {
  database_.reset(new sqlite::Database(kDbPath_,
                                        sqlite::Mode::kReadWriteCreate));
  std::string query(
      "CREATE TABLE IF NOT EXISTS DataManagerAccounts ("
      "ChunkName TEXT  PRIMARY KEY NOT NULL, PmidNodes TEXT NOT NULL);");
  sqlite::Transaction transaction{*database_};
  sqlite::Statement statement{*database_, query};
  statement.Step();
  transaction.Commit();
}

DataManagerDatabase::~DataManagerDatabase() {
  try {
    database_.reset();
    boost::filesystem::remove_all(kDbPath_);
  }
  catch (std::exception e) {
    LOG(kError) << "Failed to remove db : " << boost::diagnostic_information(e);
  }
}

void DataManagerDatabase::CheckPoint() {
  if (++write_operations_ > 1000) {
    database_->CheckPoint();
    write_operations_ = 0;
  }
}

}  // namespace vault

}  // namespace maidsafe
