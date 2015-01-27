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

#ifndef MAIDSAFE_VAULT_MPID_MANAGER_DATABASE_H_
#define MAIDSAFE_VAULT_MPID_MANAGER_DATABASE_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "maidsafe/common/sqlite3_wrapper.h"

#include "maidsafe/routing/routing_api.h"

#include "maidsafe/vault/mpid_manager/mpid_manager.h"

namespace maidsafe {

namespace vault {

class MpidManagerDatabase {
 public:
  explicit MpidManagerDatabase(const boost::filesystem::path& db_path);
  ~MpidManagerDatabase();

  void Put(const MpidManager::MessageKey& key,
           const uint32_t size,
           const MpidManager::GroupName& mpid);
  void Delete(const MpidManager::MessageKey& key);
  bool Has(const MpidManager::MessageKey& key) const;

  MpidManager::DbTransferInfo GetTransferInfo(
      std::shared_ptr<routing::CloseNodesChange> close_nodes_change);

  bool HasGroup(const MpidManager::GroupName& mpid) const;
  std::pair<uint32_t, uint32_t> GetStatistic(const MpidManager::GroupName& mpid) const;
  std::vector<MpidManager::MessageKey> GetEntriesForMPID(const MpidManager::GroupName& mpid) const;

 private:
  void DeleteGroup(const std::string& mpid);
  void PutIntoTransferInfo(const NodeId& new_holder,
                           const std::string& group_name_string,
                           const std::string& key_string,
                           MpidManager::DbTransferInfo& transfer_info);

  MpidManager::MessageKey ComposeKey(const std::string& chunk_name) const {
    return MpidManager::MessageKey(Identity(HexDecode(chunk_name)));
  }
  std::string EncodeKey(const MpidManager::MessageKey& key) const {
    return HexEncode(key->string());
  }
  MpidManager::GroupName ComposeGroupName(const std::string& mpid) const {
    return MpidManager::GroupName(Identity(HexDecode(mpid)));
  }
  std::string EncodeGroupName(const MpidManager::GroupName& group_name) const {
    return HexEncode(group_name->string());
  }

  void CheckPoint();

  std::unique_ptr<sqlite::Database> db_;
  const boost::filesystem::path kDbPath_;
  int write_operations_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MPID_MANAGER_DATABASE_H_

