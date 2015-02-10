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
      db_(UniqueDbPath(vault_root_dir)) {}

void MpidManagerHandler::Put(const ImmutableData& data, const MpidName& mpid) {
  PutChunk(data);
  db_.Put(data.name(), static_cast<uint32_t>(data.data().string().size()), mpid);
}

void MpidManagerHandler::Delete(const ImmutableData::Name& data_name) {
  DeleteChunk(data_name);
  db_.Delete(data_name);
}

bool MpidManagerHandler::Has(const ImmutableData::Name& data_name) const {
  return db_.Has(data_name);
}

bool MpidManagerHandler::HasAccount(const MpidName& mpid) const {
  return db_.HasGroup(mpid);
}

DbMessageQueryResult MpidManagerHandler::GetMessage(const ImmutableData::Name& data_name) const {
  try {
    return nfs_vault::MpidMessage(GetChunk<ImmutableData>(data_name).data().string());
  }
  catch (const maidsafe_error& error) {
    return boost::make_unexpected(error);
  }
}

}  // namespace vault

}  // namespace maidsafe
