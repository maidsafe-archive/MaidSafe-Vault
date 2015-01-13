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

#ifndef MAIDSAFE_VAULT_MPID_MANAGER_DATABASE_H_
#define MAIDSAFE_VAULT_MPID_MANAGER_DATABASE_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "maidsafe/common/sqlite3_wrapper.h"

#include "maidsafe/vault/data_manager/data_manager.h"

namespace maidsafe {

namespace vault {

class MpidManagerDataBase {
 public:
  explicit MpidManagerDataBase(const boost::filesystem::path& db_path);
  ~MpidManagerDataBase();

//  std::unique_ptr<MpidManager::Value> Commit(const MpidManager::Key& key,
//      std::function<detail::DbAction(std::unique_ptr<MpidManager::Value>& value)> functor);
//  MpidManager::Value Get(const MpidManager::Key& key);

//  std::map<MpidManager::Key, MpidManager::Value> GetRelatedAccounts(const PmidName& pmid_name);
//  MpidManager::TransferInfo GetTransferInfo(
//      std::shared_ptr<routing::CloseNodesChange> close_nodes_change);
//  void HandleTransfer(const std::vector<MpidManager::KvPair>& contents);

// private:
//  void Put(const MpidManager::Key& key, const MpidManager::Value& value);
//  void Delete(const MpidManager::Key& key);

//  MpidManager::Value ComposeValue(const std::string& chunk_size, const std::string& pmids) const;
//  MpidManager::Key ComposeKey(const std::string& chunk_name) const {
//    return MpidManager::Key(MpidManager::Key::FixedWidthString(HexDecode(chunk_name)));
//  }
//  std::string EncodeKey(const MpidManager::Key& key) const {
//    return HexEncode(key.ToFixedWidthString().string());
//  }

  void CheckPoint();

  std::unique_ptr<sqlite::Database> data_base_;
  const boost::filesystem::path kDbPath_;
  int write_operations_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MPID_MANAGER_DATABASE_H_

