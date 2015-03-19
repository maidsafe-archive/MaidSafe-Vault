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

#ifndef MAIDSAFE_VAULT_MPID_MANAGER_HANDLER_H_
#define MAIDSAFE_VAULT_MPID_MANAGER_HANDLER_H_

#include <string>
#include <vector>
#include "boost/filesystem.hpp"
#include "boost/expected/expected.hpp"

#include "maidsafe/common/visualiser_log.h"

#include "maidsafe/vault/chunk_store.h"
#include "maidsafe/vault/mpid_manager/database.h"
#include "maidsafe/vault/mpid_manager/messages.h"

namespace maidsafe {

namespace vault {

using MpidName = Identity;

using DbMessageQueryResult = boost::expected<MpidMessage, maidsafe_error>;
using DbDataQueryResult = boost::expected<ImmutableData, maidsafe_error>;

class MpidManagerHandler {
 public:
  MpidManagerHandler(const boost::filesystem::path& vault_root_dir, DiskUsage max_disk_usage);

  void Put(const ImmutableData& data, const MpidName& mpid);
  void Delete(const MessageIdType& message_id);

  DbMessageQueryResult GetMessage(const MessageIdType& message_id) const;
  DbDataQueryResult GetData(const Data::NameAndTypeId& data_name) const;
  bool Has(const MessageIdType& message_id);
  bool HasAccount(const MpidName& mpid);

  void CreateAccount(const MpidName& mpid, const NonEmptyString& mpid_account);
  void UpdateAccount(const MpidName& mpid, const NonEmptyString& mpid_account);
  void RemoveAccount(const MpidName& mpid);

//  MpidManager::TransferInfo GetTransferInfo(
//      std::shared_ptr<routing::CloseNodesChange> close_nodes_change);

 private:
  ImmutableData GetChunk(const Data::NameAndTypeId& data_name) const;

  void PutChunk(const ImmutableData& data);

  void DeleteChunk(const Data::NameAndTypeId& data_name);

  ChunkStore chunk_store_;
  MpidManagerDatabase db_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MPID_MANAGER_HANDLER_H_
