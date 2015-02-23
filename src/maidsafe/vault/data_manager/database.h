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

#ifndef MAIDSAFE_VAULT_DATA_MANAGER_DATABASE_H_
#define MAIDSAFE_VAULT_DATA_MANAGER_DATABASE_H_

#include "maidsafe/common/sqlite3_wrapper.h"

#include "maidsafe/routing/types.h"

#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

class DataManagerDatabase {
 public:
  using GetPmidsResult = boost::expected<std::vector<routing::Address>, maidsafe_error>;
  DataManagerDatabase(boost::filesystem::path db_path);
  ~DataManagerDatabase();

  template <typename DataType>
  bool Exist(const typename DataType::Name& name);

  template <typename DataType>
  void Put(const typename DataType::Name& name, const std::vector<routing::Address>& pmid_nodes);

  template <typename DataType>
  void ReplacePmidNodes(const typename DataType::Name& name,
                        const std::vector<routing::Address>& pmid_nodes);

  template <typename DataType>
  maidsafe_error RemovePmid(const typename DataType::Name& name,
                            const routing::DestinationAddress& remove_pmid);

  template <typename DataType>
  GetPmidsResult GetPmids(const typename DataType::Name& name);

 private:
  void CheckPoint();

  std::unique_ptr<sqlite::Database> database_;
  const boost::filesystem::path kDbPath_;
  int write_operations_;
};

template <typename DataType>
void DataManagerDatabase::Put(const typename DataType::Name& name,
                              const std::vector<routing::Address>& pmid_nodes) {
  if (!database_)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::db_not_present));

  std::string pmids_str;

  CheckPoint();
  sqlite::Transaction transaction{*database_};
  std::string query(
      "INSERT OR REPLACE INTO DataManagerAccounts (ChunkName, PmidNodes) VALUES (?, ?)");
  sqlite::Statement statement{*database_, query};
  statement.BindText(1, EncodeToString<DataType>(name));
  for (const auto& pmid_node : pmid_nodes)
    pmids_str += NodeId(pmid_node.string()).string();
  statement.BindText(2, pmids_str);
  statement.Step();
  transaction.Commit();
}

template <typename DataType>
void DataManagerDatabase::ReplacePmidNodes(const typename DataType::Name& name,
                                           const std::vector<routing::Address>& pmid_nodes) {
  Put<DataType>(name, pmid_nodes);
}

template <typename DataType>
maidsafe_error DataManagerDatabase::RemovePmid(const typename DataType::Name& name,
                                               const routing::DestinationAddress& remove_pmid) {
  auto result(GetPmids<DataType>(name));
  if (!result.valid())
     return result.error();

  auto& pmid_nodes(*result);
  if (std::any_of(pmid_nodes.begin(), pmid_nodes.end(),
                  [&](const routing::Address& pmid_node) {
                    return pmid_node == remove_pmid.first.data;
                  })) {
    pmid_nodes.erase(std::remove(pmid_nodes.begin(), pmid_nodes.end(), remove_pmid.first.data),
                     pmid_nodes.end());
    ReplacePmidNodes<DataType>(name, pmid_nodes);
    return maidsafe_error(CommonErrors::success);
  }
  return maidsafe_error(CommonErrors::no_such_element);
}

template <typename DataType>
DataManagerDatabase::GetPmidsResult
DataManagerDatabase::GetPmids(const typename DataType::Name& name) {
  if (!database_)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::db_not_present));

  std::vector<routing::Address> pmid_nodes;
  std::string query("SELECT PmidNodes FROM DataManagerAccounts WHERE ChunkName = ?");
  sqlite::Statement statement{*database_, query};
  statement.BindText(1, EncodeToString<DataType>(name));

  if (statement.Step() == sqlite::StepResult::kSqliteRow) {
    assert(statement.ColumnText(0).size() % NodeId::kSize == 0);
    size_t pmids_count(statement.ColumnText(0).size() / NodeId::kSize);
    for (size_t index(0); index < pmids_count; ++index)
      pmid_nodes.emplace_back(NodeId(statement.ColumnText(0).substr(index * NodeId::kSize,
                                                                    NodeId::kSize)));
  } else {
    return boost::make_unexpected(MakeError(VaultErrors::no_such_account));
  }
  return pmid_nodes;
}

template <typename DataType>
bool DataManagerDatabase::Exist(const typename DataType::Name& name) {
  if (!database_)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::db_not_present));

  std::string query("SELECT Count(*) FROM DataManagerAccounts WHERE ChunkName = ?");
  sqlite::Statement statement{*database_, query};
  statement.BindText(1, EncodeToString<DataType>(name));

  if (statement.Step() == sqlite::StepResult::kSqliteRow) {
    auto count(std::stoul(statement.ColumnText(0)));
    assert(count <= 1);
    return (count > 0);
  }
  return false;
}

}  // namespace vault

}  // namespace maidsafe

#endif // MAIDSAFE_VAULT_DATA_MANAGER_DATABASE_H_

