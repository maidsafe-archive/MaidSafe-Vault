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

#include "maidsafe/common/visualiser_log.h"
#include "maidsafe/vault/memory_fifo.h"
#include "maidsafe/vault/chunk_store.h"
#include "maidsafe/common/data_types/data_name_variant.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/vault/types.h"

#include "maidsafe/vault/mpid_manager/database.h"

namespace maidsafe {

namespace vault {

class MpidManagerHandler {
 public:
  MpidManagerHandler(const boost::filesystem::path vault_root_dir, DiskUsage max_disk_usage);

  void Put(const ImmutableData& data, const MpidName& mpid);
  void Delete(const ImmutableData::Name& data_name);
  DbMessageQueryResult GetMessage(const ImmutableData::Name& data_name) const;
  bool Has(const ImmutableData::Name& data_name) const;
  bool HasAccount(const MpidName& mpid) const;

 private:
  template <typename Data>
  Data GetChunk(const typename Data::Name& data_name) const;

  template <typename Data>
  void PutChunk(const Data& data);

  template <typename DataName>
  void DeleteChunk(const DataName& data_name);

  ChunkStore chunk_store_;
  MpidManagerDatabase db_;
};

template <typename Data>
Data MpidManagerHandler::GetChunk(const typename Data::Name& data_name) const {
  DataNameVariant data_name_variant(data_name);
  try {
    Data data(data_name,
              typename Data::serialised_type(chunk_store_.Get(data_name_variant)));
    return data;
  }
  catch (const maidsafe_error& /*error*/) {
    throw;
  }
}

template <typename Data>
void MpidManagerHandler::PutChunk(const Data& data) {
  VLOG(nfs::Persona::kPmidNode, VisualiserAction::kStoreChunk, data.name().value);
  chunk_store_.Put(DataNameVariant(data.name()), data.Serialise().data);
}

template <typename DataName>
void MpidManagerHandler::DeleteChunk(const DataName& data_name) {
  chunk_store_.Delete(DataNameVariant(data_name));
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MPID_MANAGER_HANDLER_H_
