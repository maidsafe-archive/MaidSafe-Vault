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

#include "maidsafe/vault/mpid_manager/database.h"

#include <utility>
#include <cstdint>
#include <string>

#include "boost/filesystem.hpp"
#include "boost/algorithm/string.hpp"

#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/common/sqlite3_wrapper.h"

#include "maidsafe/vault/data_manager/data_manager.h"

namespace maidsafe {

namespace vault {

MpidManagerDataBase::MpidManagerDataBase(const boost::filesystem::path& db_path)
  : data_base_(), kDbPath_(db_path), write_operations_(0) {
  data_base_.reset(new sqlite::Database(db_path,
                                        sqlite::Mode::kReadWriteCreate));
  std::string query(
      "CREATE TABLE IF NOT EXISTS MpidManagerAccounts ("
      "Chunk_Name TEXT  PRIMARY KEY NOT NULL, Chunk_Size TEXT NOT NULL,"
      "Storage_Nodes TEXT NOT NULL);");
  sqlite::Transaction transaction{*data_base_};
  sqlite::Statement statement{*data_base_, query};
  statement.Step();
  transaction.Commit();
}

MpidManagerDataBase::~MpidManagerDataBase() {
  try {
    data_base_.reset();
    boost::filesystem::remove_all(kDbPath_);
  }
  catch (const std::exception& e) {
    LOG(kError) << "Failed to remove db : " << boost::diagnostic_information(e);
  }
}

}  // namespace vault

}  // namespace maidsafe

