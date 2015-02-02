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

#include "maidsafe/vault/mpid_manager/database.h"

#include <utility>
#include <cstdint>
#include <string>

#include "boost/filesystem.hpp"
#include "boost/algorithm/string.hpp"

#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/common/sqlite3_wrapper.h"
#include "maidsafe/common/serialisation/serialisation.h"

namespace maidsafe {

namespace vault {

MpidManagerDatabase::MpidManagerDatabase(const boost::filesystem::path& db_path)
    : db_(), kDbPath_(db_path), write_operations_(0) {
  db_.reset(new sqlite::Database(db_path,
                                        sqlite::Mode::kReadWriteCreate));
  std::string query(
      "CREATE TABLE IF NOT EXISTS MpidManagerAccounts ("
      "Chunk_Name TEXT  PRIMARY KEY NOT NULL, Chunk_Size TEXT NOT NULL,"
      "MPID TEXT NOT NULL);");
  sqlite::Transaction transaction{*db_};
  sqlite::Statement statement{*db_, query};
  statement.Step();
  transaction.Commit();
}

MpidManagerDatabase::~MpidManagerDatabase() {
  try {
    db_.reset();
    boost::filesystem::remove_all(kDbPath_);
  }
  catch (const std::exception& e) {
    LOG(kError) << "Failed to remove db : " << boost::diagnostic_information(e);
  }
}

void MpidManagerDatabase::Put(const MpidManager::MessageKey& key,
                              const uint32_t size,
                              const MpidManager::GroupName& group_name) {
  if (!db_)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::db_not_present));
  CheckPoint();

  sqlite::Transaction transaction{*db_};
  std::string query(
      "INSERT OR REPLACE INTO MpidManagerAccounts (Chunk_Name, Chunk_size,"
      " MPID) VALUES (?, ?, ?)");
  sqlite::Statement statement{*db_, query};

  std::string mpid(EncodeGroupName(group_name));
  statement.BindText(3, mpid);
  std::string chunk_size(ConvertToString(size));
  statement.BindText(2, chunk_size);
  std::string chunk_name(EncodeKey(key));
  statement.BindText(1, chunk_name);

  statement.Step();
  transaction.Commit();
}

void MpidManagerDatabase::Delete(const MpidManager::MessageKey& key) {
  if (!db_)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::db_not_present));
  CheckPoint();

  sqlite::Transaction transaction{*db_};
  std::string query("DELETE FROM MpidManagerAccounts WHERE Chunk_Name=?");
  sqlite::Statement statement{*db_, query};
  auto key_string(EncodeKey(key));
  statement.BindText(1, key_string);
  statement.Step();
  transaction.Commit();
}

void MpidManagerDatabase::DeleteGroup(const std::string& mpid) {
  if (!db_)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::db_not_present));
  CheckPoint();

  sqlite::Transaction transaction{*db_};
  std::string query("DELETE FROM MpidManagerAccounts WHERE MPID=?");
  sqlite::Statement statement{*db_, query};
  statement.BindText(1, mpid);
  statement.Step();
  transaction.Commit();
}

MpidManager::DbTransferInfo MpidManagerDatabase::GetTransferInfo(
    std::shared_ptr<routing::CloseNodesChange> close_nodes_change) {
  if (!db_)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::db_not_present));

  std::vector<std::pair<NodeId, std::string>> groups_to_be_transferred;
  std::vector<std::string> groups_to_be_removed;
  {
    std::string query("SELECT DISTINCT MPID from MpidManagerAccounts");
    sqlite::Statement statement{*db_, query};
    while (statement.Step() == sqlite::StepResult::kSqliteRow) {
      std::string group_name(statement.ColumnText(0));
      MpidManager::GroupName mpid(ComposeGroupName(group_name));
      auto check_holder_result = close_nodes_change->CheckHolders(NodeId(mpid->string()));
      if (check_holder_result.proximity_status == routing::GroupRangeStatus::kInRange) {
        if (check_holder_result.new_holder == NodeId())
          continue;
        groups_to_be_transferred.push_back(std::make_pair(check_holder_result.new_holder,
                                                          group_name));
      } else {
  //      VLOG(VisualiserAction::kRemoveAccount, key.name);
        groups_to_be_removed.push_back(group_name);
        // empty NodeId indicates removing from local
        groups_to_be_transferred.push_back(std::make_pair(NodeId(), group_name));
      }
    }
  }

  MpidManager::DbTransferInfo transfer_info;
  for (const auto& transfer_entry : groups_to_be_transferred) {
    std::string query("SELECT Chunk_Name from MpidManagerAccounts WHERE MPID=?");
    sqlite::Statement statement{*db_, query};
    statement.BindText(1, transfer_entry.second);
    while (statement.Step() == sqlite::StepResult::kSqliteRow)
      PutIntoTransferInfo(transfer_entry.first, transfer_entry.second,
                          statement.ColumnText(0), transfer_info);
  }

  for (const auto& group_name : groups_to_be_removed)
    DeleteGroup(group_name);

  return transfer_info;
}

void MpidManagerDatabase::PutIntoTransferInfo(const NodeId& new_holder,
                                              const std::string& group_name_string,
                                              const std::string& key_string,
                                              MpidManager::DbTransferInfo& transfer_info) {
  MpidManager::GroupName group_name(ComposeGroupName(group_name_string));
  MpidManager::MessageKey key(ComposeKey(key_string));
  auto found_itr = transfer_info.find(new_holder);
  if (found_itr != transfer_info.end()) {  // append
    found_itr->second.push_back(std::make_pair(group_name, key));
  } else {  // create
    std::vector<MpidManager::GKPair> group_key_vector;
    group_key_vector.push_back(std::make_pair(group_name, key));
    transfer_info.insert(std::make_pair(new_holder, std::move(group_key_vector)));
  }
}

bool MpidManagerDatabase::HasGroup(const MpidManager::GroupName& mpid) const {
  if (!db_)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::db_not_present));
  std::string query("SELECT DISTINCT MPID from MpidManagerAccounts WHERE MPID=?");
  sqlite::Statement statement{*db_, query};
  auto group_name(EncodeGroupName(mpid));
  statement.BindText(1, group_name);
  return (statement.Step() == sqlite::StepResult::kSqliteRow);
}

bool MpidManagerDatabase::Has(const MpidManager::MessageKey& key) const {
  if (!db_)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::db_not_present));
  std::string query("SELECT * FROM MpidManagerAccounts WHERE Chunk_Name=?");
  sqlite::Statement statement{*db_, query};
  auto key_string(EncodeKey(key));
  statement.BindText(1, key_string);
  return (statement.Step() == sqlite::StepResult::kSqliteRow);
}

std::pair<uint32_t, uint32_t> MpidManagerDatabase::GetStatistic(
    const MpidManager::GroupName& mpid) const {
  if (!db_)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::db_not_present));
  uint32_t num_of_messages(0), total_size(0);
  std::string query("SELECT Chunk_Size from MpidManagerAccounts WHERE MPID=?");
  sqlite::Statement statement{*db_, query};
  auto group_name(EncodeGroupName(mpid));
  statement.BindText(1, group_name);
  while (statement.Step() == sqlite::StepResult::kSqliteRow) {
    ++num_of_messages;
    total_size += std::stoi(statement.ColumnText(0));
  }
  return std::make_pair(num_of_messages, total_size);
}

std::vector<MpidManager::MessageKey> MpidManagerDatabase::GetEntriesForMPID(
    const MpidManager::GroupName& mpid) const {
  if (!db_)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::db_not_present));
  std::vector<MpidManager::MessageKey> entries;
  std::string query("SELECT Chunk_Name from MpidManagerAccounts WHERE MPID=?");
  sqlite::Statement statement{*db_, query};
  auto group_name(EncodeGroupName(mpid));
  statement.BindText(1, group_name);
  while (statement.Step() == sqlite::StepResult::kSqliteRow)
    entries.push_back(ComposeKey(statement.ColumnText(0)));
  return entries;
}

void MpidManagerDatabase::CheckPoint() {
  if (++write_operations_ > 1000) {
    db_->CheckPoint();
    write_operations_ = 0;
  }
}

}  // namespace vault

}  // namespace maidsafe
