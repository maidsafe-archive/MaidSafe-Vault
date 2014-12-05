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

#include "maidsafe/vault/data_manager/database.h"

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

DataManagerDataBase::DataManagerDataBase(const boost::filesystem::path& db_path)
  : data_base_(), kDbPath_(db_path), write_operations_(0) {
  data_base_.reset(new sqlite::Database(db_path,
                                        sqlite::Mode::kReadWriteCreate));
  std::string query(
      "CREATE TABLE IF NOT EXISTS DataManagerAccounts ("
      "Chunk_Name TEXT  PRIMARY KEY NOT NULL, Chunk_Size TEXT NOT NULL, Storage_Nodes TEXT NOT NULL);");
  sqlite::Transaction transaction{*data_base_};
  sqlite::Statement statement{*data_base_, query};
  statement.Step();
  transaction.Commit();
}

DataManagerDataBase::~DataManagerDataBase() {
  try {
    data_base_.reset();
    boost::filesystem::remove_all(kDbPath_);
  }
  catch (const std::exception& e) {
    LOG(kError) << "Failed to remove db : " << boost::diagnostic_information(e);
  }
}

std::unique_ptr<DataManager::Value> DataManagerDataBase::Commit(const DataManager::Key& key,
    std::function<detail::DbAction(std::unique_ptr<DataManager::Value>& value)> functor) {
  assert(functor);
  std::unique_ptr<DataManager::Value> value;
  try {
    value.reset(new DataManager::Value(Get(key)));
  }
  catch (const maidsafe_error& error) {
    if (error.code() != make_error_code(VaultErrors::no_such_account)) {
      LOG(kError) << "DataManagerDataBase::Commit unknown db error "
                  << boost::diagnostic_information(error);
      throw;  // For db errors
    }
  }
  if (detail::DbAction::kPut == functor(value)) {
    assert(value);
    LOG(kInfo) << "DataManagerDataBase::Commit putting entry";
    Put(key, std::move(*value));
  } else {
    LOG(kInfo) << "DataManagerDataBase::Commit deleting entry";
    if (!value)
      BOOST_THROW_EXCEPTION(MakeError(CommonErrors::null_pointer));
    Delete(key);
    return value;
  }
  return nullptr;
}

void DataManagerDataBase::Put(const DataManager::Key& key, const DataManager::Value& value) {
  if (!data_base_)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::db_not_presented));
  CheckPoint();

  sqlite::Transaction transaction{*data_base_};
  std::string query(
      "INSERT OR REPLACE INTO DataManagerAccounts (Chunk_Name, Chunk_size, Storage_Nodes) VALUES (?, ?, ?)");
  sqlite::Statement statement{*data_base_, query};

  std::string storage_nodes;
  for (auto& storage_node : value.AllPmids())
    storage_nodes += NodeId(storage_node->string()).ToStringEncoded(NodeId::EncodingType::kHex) + ";";
//  LOG(kVerbose) << "inserting pmids as " << storage_nodes;
  statement.BindText(3, storage_nodes);

  std::string chunk_size(std::to_string(value.chunk_size()));
//  LOG(kVerbose) << "inserting chunk size as " << chunk_size;
  statement.BindText(2, chunk_size);

  std::string chunk_name(EncodeKey(key));
//  LOG(kVerbose) << "inserting chunk_name as " << chunk_name;
  statement.BindText(1, chunk_name);

  statement.Step();
  transaction.Commit();
}

DataManager::Value DataManagerDataBase::Get(const DataManager::Key& key) {
  if (!data_base_)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::db_not_presented));
  DataManager::Value value;
  std::string query(
      "SELECT Chunk_Size, Storage_Nodes FROM DataManagerAccounts WHERE Chunk_Name=?");
  sqlite::Statement statement{*data_base_, query};
  auto chunk_name(EncodeKey(key));
  LOG(kVerbose) << "looking for chunk " << chunk_name;
  statement.BindText(1, chunk_name);
  if (statement.Step() == sqlite::StepResult::kSqliteRow) {
    value = ComposeValue(statement.ColumnText(0), statement.ColumnText(1));
  } else {
    LOG(kWarning) << "dones't got account for chunk " << EncodeKey(key);
    BOOST_THROW_EXCEPTION(MakeError(VaultErrors::no_such_account));
  }
  return value;
}

void DataManagerDataBase::Delete(const DataManager::Key& key) {
  if (!data_base_)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::db_not_presented));
  CheckPoint();

  sqlite::Transaction transaction{*data_base_};
  std::string query("DELETE FROM DataManagerAccounts WHERE Chunk_Name=?");
  sqlite::Statement statement{*data_base_, query};
  auto key_string(EncodeKey(key));
  statement.BindText(1, key_string);
  statement.Step();
  transaction.Commit();
}

std::map<DataManager::Key, DataManager::Value> DataManagerDataBase::GetRelatedAccounts(
    const PmidName& pmid_name) {
  if (!data_base_)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::db_not_presented));

  std::map<DataManager::Key, DataManager::Value> result;
  std::string query(
      "SELECT * FROM DataManagerAccounts WHERE Storage_Nodes LIKE ?");
  sqlite::Statement statement{*data_base_, query};
  std::string target('%' + NodeId(pmid_name->string()).ToStringEncoded(
                        NodeId::EncodingType::kHex) + '%');
  statement.BindText(1, target);
  while (statement.Step() == sqlite::StepResult::kSqliteRow) {
    DataManager::Key key(ComposeKey(statement.ColumnText(0)));
    DataManagerValue value(ComposeValue(statement.ColumnText(1), statement.ColumnText(2)));
    result[key] = value;
  }
  return std::move(result);
}

DataManager::TransferInfo DataManagerDataBase::GetTransferInfo(
    std::shared_ptr<routing::CloseNodesChange> close_nodes_change) {
  if (!data_base_)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::db_not_presented));

  std::vector<DataManager::Key> prune_vector;
  DataManager::TransferInfo transfer_info;

  std::string query("SELECT * from DataManagerAccounts");
  sqlite::Statement statement{*data_base_, query};
  while (statement.Step() == sqlite::StepResult::kSqliteRow) {
    DataManager::Key key(ComposeKey(statement.ColumnText(0)));
    DataManager::Value value(ComposeValue(statement.ColumnText(1), statement.ColumnText(2)));

    auto check_holder_result = close_nodes_change->CheckHolders(NodeId(key.name.string()));
    if (check_holder_result.proximity_status == routing::GroupRangeStatus::kInRange) {
      LOG(kVerbose) << "Db::GetTransferInfo in range";
      if (check_holder_result.new_holder == NodeId())
        continue;
      LOG(kVerbose) << "Db::GetTransferInfo having new holder " << check_holder_result.new_holder;
      auto found_itr = transfer_info.find(check_holder_result.new_holder);
      if (found_itr != transfer_info.end()) {
        LOG(kInfo) << "Db::GetTransferInfo add into transfering account "
                   << HexSubstr(key.name.string()) << " to " << check_holder_result.new_holder;
        found_itr->second.push_back(std::make_pair(key, value));
      } else {  // create
        LOG(kInfo) << "Db::GetTransferInfo create transfering account "
                   << HexSubstr(key.name.string()) << " to " << check_holder_result.new_holder;
        std::vector<DataManager::KvPair> kv_pair;
        kv_pair.push_back(std::make_pair(key, value));
        transfer_info.insert(std::make_pair(check_holder_result.new_holder, std::move(kv_pair)));
      }
    } else {
//      VLOG(VisualiserAction::kRemoveAccount, key.name);
      prune_vector.push_back(key);
    }
  }
  for (const auto& key : prune_vector)
    Delete(key);  // Ignore Delete failure here ?
  return transfer_info;
}

void DataManagerDataBase::HandleTransfer(const std::vector<DataManager::KvPair>& contents) {
  LOG(kVerbose) << "DataManager AcoccountTransfer DataManagerDataBase::HandleTransfer";
  for (const auto& kv_pair : contents) {
    try {
      Get(kv_pair.first);
    }
    catch (const maidsafe_error& error) {
      LOG(kInfo) << "DataManager AcoccountTransfer DataManagerDataBase::HandleTransfer "
                 << error.what();
      if ((error.code() != make_error_code(CommonErrors::no_such_element)) &&
          (error.code() != make_error_code(VaultErrors::no_such_account))) {
        throw;  // For db errors
      } else {
        LOG(kInfo) << "DataManager AcoccountTransfer DataManagerDataBase::HandleTransfer "
                   << "inserting account " << HexSubstr(kv_pair.first.name.string());
        Put(kv_pair.first, kv_pair.second);
      }
    }
  }
}

DataManager::Value DataManagerDataBase::ComposeValue(const std::string& chunk_size,
                                                     const std::string& pmids) const {
  DataManager::Value value;
//  LOG(kVerbose) << "chunk_size is " << chunk_size;
  value.SetChunkSize(std::stoi(chunk_size));
  std::vector<std::string> storage_nodes;
//  LOG(kVerbose) << "pmids are " << pmids;
  // The following line will throw an error msg to be printed out on screen :
  // find_iterator.hpp:243:30: runtime error: load of value ...
  boost::algorithm::split(storage_nodes, pmids, boost::is_any_of(";"));
  storage_nodes.pop_back();
  for (auto& storage_node : storage_nodes)
    value.AddPmid(PmidName(Identity(NodeId(storage_node, NodeId::EncodingType::kHex).string())));
  return value;
}

void DataManagerDataBase::CheckPoint() {
  if (++write_operations_ > 1000) {
    data_base_->CheckPoint();
    write_operations_ = 0;
  }
}

}  // namespace vault

}  // namespace maidsafe

