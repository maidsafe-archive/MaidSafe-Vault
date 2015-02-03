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

#include "maidsafe/vault/mpid_manager/handler.h"

#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

MpidManagerHandler::MpidManagerHandler(const boost::filesystem::path vault_root_dir,
                                       DiskUsage max_disk_usage)
    : chunk_store_(vault_root_dir / "mpid_manager" / "permanent", max_disk_usage),
      db_() {}

void MpidManagerHandler::Put(const ImmutableData& data, const MpidName& mpid) {
  PutChunk(data);
  db_.Put(data.name(), static_cast<uint32_t>(data.data().string().size()), mpid);
}

void MpidManagerHandler::Delete(const ImmutableData::Name& data_name) {
  DeleteChunk(data_name);
  db_.Delete(data_name);
}

bool MpidManagerHandler::Has(const ImmutableData::Name& data_name) {
  return db_.Has(data_name);
}

bool MpidManagerHandler::HasAccount(const MpidName& mpid) {
  return db_.HasGroup(mpid);
}

// mpid_account becomes a special entry in database with chunk_size to be 0
// this allows no additional action need to be undertaken during account_transfer
// and also allows update and query of account to be possbile
// (once proper mpid_account struct got defined)
void MpidManagerHandler::CreateAccount(const MpidName& mpid, const NonEmptyString& mpid_account) {
  if (HasAccount(mpid))
    BOOST_THROW_EXCEPTION(MakeError(VaultErrors::account_already_exists));
  ImmutableData data(mpid_account);
  PutChunk(data);
  db_.Put(data.name(), 0, mpid);
}

void MpidManagerHandler::UpdateAccount(const MpidName& mpid, const NonEmptyString& mpid_account) {
  if (!HasAccount(mpid))
    BOOST_THROW_EXCEPTION(MakeError(VaultErrors::no_such_account));
  auto prev_account_name(db_.GetAccountChunkName(mpid));
  Delete(prev_account_name);
  ImmutableData data(mpid_account);
  PutChunk(data);
  db_.Put(data.name(), 0, mpid);
}

void MpidManagerHandler::RemoveAccount(const MpidName& mpid) {
  auto entries(db_.GetEntriesForMPID(mpid));
  for (const auto& entry : entries)
    Delete(entry);
}

DbMessageQueryResult MpidManagerHandler::GetMessage(const ImmutableData::Name& data_name) const {
  try {
    return nfs_vault::MpidMessage(GetChunk<ImmutableData>(data_name).data().string());
  }
  catch (const maidsafe_error& error) {
    return boost::make_unexpected(error);
  }
}

DbDataQueryResult MpidManagerHandler::GetData(const ImmutableData::Name& data_name) const {
  try {
    return GetChunk<ImmutableData>(data_name);
  }
  catch (const maidsafe_error& error) {
    return boost::make_unexpected(error);
  }
}

MpidManager::TransferInfo MpidManagerHandler::GetTransferInfo(
    std::shared_ptr<routing::CloseNodesChange> close_nodes_change) {
  MpidManager::DbTransferInfo db_transfer_info(db_.GetTransferInfo(close_nodes_change));
  auto prune_itr(db_transfer_info.find(NodeId()));
  if (prune_itr != db_transfer_info.end()) {
    for (const auto& prune_entry : prune_itr->second) {
      try {
        Delete(prune_entry.second);
      } catch (const maidsafe_error& error) {
        LOG(kError) << "MpidManagerHandler::GetTransferInfo got error " << error.what()
                    << " when deleting chunk " << HexSubstr(prune_entry.second->string());
      }
    }
    db_transfer_info.erase(prune_itr);
  }

  MpidManager::TransferInfo transfer_info;
  for (const auto& transfer : db_transfer_info) {
    std::vector<MpidManager::KVPair> kv_pairs;
    for (const auto& account_entry : transfer.second) {
      try {
        kv_pairs.push_back(std::make_pair(account_entry.first,
            MpidManager::Value(GetChunk<ImmutableData>(account_entry.second))));
      } catch (const maidsafe_error& error) {
        LOG(kError) << "MpidManagerHandler::GetTransferInfo got error " << error.what()
                    << " when fetching chunk " << HexSubstr(account_entry.second->string());
      }
    }
    transfer_info.insert(std::make_pair(transfer.first, std::move(kv_pairs)));
  }
  return transfer_info;
}

}  // namespace vault

}  // namespace maidsafe
