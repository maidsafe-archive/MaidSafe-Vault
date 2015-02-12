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

namespace maidsafe {

namespace vault {

namespace detail {
  struct PaddedWidth {
    static const int value = 1;
  };

  using FixedWidthString = maidsafe::detail::BoundedString<NodeId::kSize + PaddedWidth::value,
                                                           NodeId::kSize + PaddedWidth::value>;
  template <int width>
  std::string ToFixedWidthString(uint32_t number) {
    static_assert(width > 0 && width < 5, "width must be 1, 2, 3, or 4.");
    assert(number < std::pow(256, width));
    std::string result(width, 0);
    for (int i(0); i != width; ++i) {
      result[width - i - 1] = static_cast<char>(number);
      number /= 256;
    }
    return result;
  }

  template <>
  std::string ToFixedWidthString<1>(uint32_t number);

  template <typename DataType>
  std::string EncodeToString(typename DataType::Name name) {
    return detail::FixedWidthString(
               name.string() + ToFixedWidthString<PaddedWidth::value>(
                                   static_cast<uint32_t>(DataType::Tag::kValue))).string();
  }
}

class DataManagerDatabase {
 public:
  DataManagerDatabase(boost::filesystem::path db_path);
  ~DataManagerDatabase();

  template <typename DataType>
  bool Exist(typename DataType::Name /*name*/) {
    return true;
  }

  template <typename DataType>
  void Put(typename DataType::Name name, std::vector<routing::Address> pmid_nodes);

  template <typename DataType>
  void ReplacePmidNodes(typename DataType::Name name, std::vector<routing::Address> pmid_nodes);

  template <typename DataType>
  void RemovePmid(typename DataType::Name name, routing::Address remove_pmid);

  template <typename DataType>
  std::vector<routing::Address> GetPmids(typename DataType::Name name);

 private:
  void CheckPoint();

  std::unique_ptr<sqlite::Database> database_;
  const boost::filesystem::path kDbPath_;
  int write_operations_;
};

template <typename DataType>
void DataManagerDatabase::Put(typename DataType::Name name,
                              std::vector<routing::Address> pmid_nodes) {
  if (!database_)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::db_not_present));

  std::string pmids_str;

  CheckPoint();
  sqlite::Transaction transaction{*database_};
  std::string query(
      "INSERT OR REPLACE INTO DataManagerAccounts (ChunkName, PmidNodes) VALUES (?, ?, ?)");
  sqlite::Statement statement{*database_, query};
  statement.BindText(1, detail::EncodeToString(name));
  for (const auto& pmid_node : pmid_nodes)
    pmids_str += NodeId(pmid_node.string()).ToStringEncoded(NodeId::EncodingType::kHex) +";";
  statement.BindText(2, pmids_str);
  statement.Step();
  transaction.Commit();
}

template <typename DataType>
void DataManagerDatabase::ReplacePmidNodes(typename DataType::Name name,
                                           std::vector<routing::Address> pmid_nodes) {
  if (!database_)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::db_not_present));

  std::string pmids_str;

  CheckPoint();
  sqlite::Transaction transaction{*database_};
  std::string query(
      "Update DataManagerAccounts SET PmidNodes = ? WHERE ChunkName = ?");
  sqlite::Statement statement{*database_, query};
  statement.BindText(1, detail::EncodeToString(name));
  for (const auto& pmid_node : pmid_nodes)
    pmids_str += NodeId(pmid_node.string()).ToStringEncoded(NodeId::EncodingType::kHex);
  statement.BindText(2, pmids_str);
  statement.Step();
  transaction.Commit();
}

template <typename DataType>
void DataManagerDatabase::RemovePmid(typename DataType::Name name, routing::Address remove_pmid) {
  auto pmid_nodes(GetPmids(name));
  if (std::any_of(pmid_nodes.begin(), pmid_nodes.end(),
                  [&](const routing::Address& pmid_node) {
                    return pmid_node == remove_pmid;
                  })) {
    pmid_nodes.erase(std::remove(pmid_nodes.begin(), pmid_nodes.end(), remove_pmid),
                     pmid_nodes.end());
    ReplacePmidNodes(name, pmid_nodes);
  }
}

template <typename DataType>
std::vector<routing::Address> DataManagerDatabase::GetPmids(typename DataType::Name name) {
  if (!database_)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::db_not_present));

  std::vector<routing::Address> pmid_nodes;

  std::string query(
      "SELECT PmidNodes FROM DataManagerAccounts WHERE ChunkName = ?");
  sqlite::Statement statement{*database_, query};
  statement.BindText(1, detail::EncodeToString(name));

  if (statement.Step() == sqlite::StepResult::kSqliteRow) {
    size_t pmids_count(statement.ColumnText(0).size() / NodeId::kSize);
    for (size_t index(0); index < pmids_count; ++index)
      pmid_nodes.emplace_back(NodeId(statement.ColumnText(0).substr(index * NodeId::kSize,
                                                                    NodeId::kSize)));
  }
  return pmid_nodes;
}

}  // namespace vault

}  // namespace maidsafe

#endif // MAIDSAFE_VAULT_DATA_MANAGER_DATABASE_H_

