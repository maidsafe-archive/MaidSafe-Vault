/***************************************************************************************************
 *  Copyright 2012 MaidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of MaidSafe.net.                                  *
 **************************************************************************************************/

#include "maidsafe/vault/data_holder/data_holder_service.h"

#include <string>

#include "maidsafe/common/utils.h"
#include "maidsafe/common/types.h"
#include "maidsafe/data_store/data_buffer.h"

#include "maidsafe/vault/pmid_account_holder/pmid_account_pb.h"


namespace maidsafe {

namespace vault {

namespace {

MemoryUsage mem_usage = MemoryUsage(524288000);  // 500Mb
MemoryUsage perm_usage = MemoryUsage(mem_usage / 5);
MemoryUsage cache_usage = MemoryUsage(mem_usage * 2 / 5);
MemoryUsage mem_only_cache_usage = MemoryUsage(mem_usage * 2 / 5);
//  boost::filesystem::space_info space = boost::filesystem::space("/tmp/vault_root_dir\\");  // FIXME  NOLINT

//  DiskUsage disk_total = DiskUsage(space.available);
//  DiskUsage permanent_size = DiskUsage(disk_total * 0.8);
//  DiskUsage cache_size = DiskUsage(disk_total * 0.1);

inline bool SenderIsConnectedVault(const nfs::DataMessage& data_message,
                                   routing::Routing& routing) {
  return routing.IsConnectedVault(data_message.source().node_id) &&
         routing.EstimateInGroup(data_message.source().node_id, routing.kNodeId());
}

inline bool SenderInGroupForMetadata(const nfs::DataMessage& data_message,
                                     routing::Routing& routing) {
  return routing.EstimateInGroup(data_message.source().node_id,
                                 NodeId(data_message.data().name.string()));
}

template<typename Message>
inline bool ForThisPersona(const Message& message) {
  return message.destination_persona() != nfs::Persona::kDataHolder;
}

}  // unnamed namespace


const int DataHolderService::kPutRequestsRequired_(3);
const int DataHolderService::kDeleteRequestsRequired_(3);

DataHolderService::DataHolderService(const passport::Pmid& pmid,
                                     routing::Routing& routing,
                                     const boost::filesystem::path& vault_root_dir)
    : space_info_(boost::filesystem::space(vault_root_dir)),
      disk_total_(space_info_.available),
      permanent_size_(disk_total_ * 4 / 5),
      cache_size_(disk_total_ / 10),
      permanent_data_store_(vault_root_dir / "data_holder" / "permanent", DiskUsage(10000)/*perm_usage*/),  // TODO(Fraser) BEFORE_RELEASE need to read value from disk
      cache_data_store_(cache_usage, DiskUsage(cache_size_ / 2), nullptr,
                        vault_root_dir / "data_holder" / "cache"),  // FIXME - DiskUsage  NOLINT
      mem_only_cache_(mem_only_cache_usage, DiskUsage(cache_size_ / 2), nullptr,
                      vault_root_dir / "data_holder" / "cache"),  // FIXME - DiskUsage should be 0  NOLINT
      routing_(routing),
      accumulator_mutex_(),
      accumulator_(),
      nfs_(routing_, pmid) {
//  nfs_.GetElementList();  // TODO (Fraser) BEFORE_RELEASE Implementation needed
}

void DataHolderService::ValidatePutSender(const nfs::DataMessage& data_message) const {
  if (!SenderIsConnectedVault(data_message, routing_))
    ThrowError(VaultErrors::permission_denied);

  if (!FromPmidAccountHolder(data_message) || !ForThisPersona(data_message))
    ThrowError(CommonErrors::invalid_parameter);
}

void DataHolderService::ValidateGetSender(const nfs::DataMessage& data_message) const {
  if (!SenderInGroupForMetadata(data_message, routing_))
    ThrowError(VaultErrors::permission_denied);

  if (!FromMetadataManager(data_message) || !ForThisPersona(data_message))
    ThrowError(CommonErrors::invalid_parameter);
}

void DataHolderService::ValidateDeleteSender(const nfs::DataMessage& data_message) const {
  if (!SenderIsConnectedVault(data_message, routing_))
    ThrowError(VaultErrors::permission_denied);

  if (!FromPmidAccountHolder(data_message) || !ForThisPersona(data_message))
    ThrowError(CommonErrors::invalid_parameter);
}

}  // namespace vault

}  // namespace maidsafe
