/* Copyright 2012 MaidSafe.net limited

This MaidSafe Software is licensed under the MaidSafe.net Commercial License, version 1.0 or later,
and The General Public License (GPL), version 3. By contributing code to this project You agree to
the terms laid out in the MaidSafe Contributor Agreement, version 1.0, found in the root directory
of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available at:

http://www.novinet.com/license

Unless required by applicable law or agreed to in writing, software distributed under the License is
distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.
*/

#include "maidsafe/vault/pmid_node/service.h"

#include <string>

#include "maidsafe/common/utils.h"
#include "maidsafe/common/types.h"
#include "maidsafe/data_store/data_buffer.h"

#include "maidsafe/vault/pmid_manager/pmid_manager.pb.h"


namespace fs = boost::filesystem;

namespace maidsafe {

namespace vault {

namespace {

MemoryUsage mem_usage = MemoryUsage(524288000);  // 500Mb
MemoryUsage perm_usage = MemoryUsage(mem_usage / 5);
MemoryUsage cache_usage = MemoryUsage(mem_usage * 2 / 5);
// MemoryUsage mem_only_cache_usage = MemoryUsage(mem_usage * 2 / 5);
MemoryUsage mem_only_cache_usage = MemoryUsage(100);  // size in elements
//  fs::space_info space = fs::space("/tmp/vault_root_dir\\");  // FIXME  NOLINT

//  DiskUsage disk_total = DiskUsage(space.available);
//  DiskUsage permanent_size = DiskUsage(disk_total * 0.8);
//  DiskUsage cache_size = DiskUsage(disk_total * 0.1);

inline bool SenderIsConnectedVault(const nfs::Message& message, routing::Routing& routing) {
  return routing.IsConnectedVault(message.source().node_id) &&
         routing.EstimateInGroup(message.source().node_id, routing.kNodeId());
}

inline bool SenderInGroupForMetadata(const nfs::Message& message, routing::Routing& routing) {
  return routing.EstimateInGroup(message.source().node_id, NodeId(message.data().name.string()));
}

template<typename Message>
inline bool ForThisPersona(const Message& message) {
  return message.destination_persona() != nfs::Persona::kPmidNode;
}

}  // unnamed namespace


PmidNodeService::PmidNodeService(const passport::Pmid& pmid,
                                     routing::Routing& routing,
                                     const fs::path& vault_root_dir)
    : space_info_(fs::space(vault_root_dir)),
      disk_total_(space_info_.available),
      permanent_size_(disk_total_ * 4 / 5),
      cache_size_(disk_total_ / 10),
      permanent_data_store_(vault_root_dir / "pmid_node" / "permanent", DiskUsage(10000)/*perm_usage*/),  // TODO(Fraser) BEFORE_RELEASE need to read value from disk
      cache_data_store_(cache_usage, DiskUsage(cache_size_ / 2), nullptr,
                        vault_root_dir / "pmid_node" / "cache"),  // FIXME - DiskUsage  NOLINT
      mem_only_cache_(mem_only_cache_usage),
      //mem_only_cache_(mem_only_cache_usage, DiskUsage(cache_size_ / 2), nullptr,
      //                vault_root_dir / "pmid_node" / "cache"),  // FIXME - DiskUsage should be 0  NOLINT
      routing_(routing),
      accumulator_mutex_(),
      accumulator_(),
      nfs_(routing_, pmid) {
//  nfs_.GetElementList();  // TODO (Fraser) BEFORE_RELEASE Implementation needed
}

void PmidNodeService::SendAccountRequest() {
}

void PmidNodeService::ApplyAccountTransfer(const size_t& /*total_pmidmgrs*/,
                                           const size_t& /*pmidmagsr_with_account*/) {
  struct ChunkInfo {
    ChunkInfo(const Identity& hash_in, const Identity& size_in) :
        hash(hash_in), size(size_in) {}
    Identity hash;
    uint64_t size;
  };
  std::map<Identity, std::vector<ChunkInfo>> pmid_account_responses;
  protobuf::PmidAccountResponse pmid_account_response;
  protobuf::PmidAccountDetails pmid_account_details;
  protobuf::PmidAccountDbEntry pmid_account_entry;
  for (auto pending_request : accumulator_.pending_requests_) {
    if (static_cast<nfs::MessageAction>(pending_request.msg.data().action) == nfs::MessageAction::kAccountTransfer) {
      pmid_account_response.ParseFromString(pending_request.msg.data().content.string());
      pmid_account_details.ParseFromString(pmid_account_response.pmid_account().serialised_account_details());
      for (int index(0); index < pmid_account_details.db_entry_size(); ++index) {
        ChunkInfo chunk_info(pmid_account_details.db_entry(index).name(),
                             pmid_account_details.db_entry(index).value().size());
        pmid_account_responses[]
      }
      pmid_account_responses[pending_request.msg.data().originator] = pmid_account_response;
    }
  }
}

void PmidNodeService::ValidatePutSender(const nfs::Message& message) const {
  if (!SenderIsConnectedVault(message, routing_))
    ThrowError(VaultErrors::permission_denied);

  if (!FromPmidManager(message) || !ForThisPersona(message))
    ThrowError(CommonErrors::invalid_parameter);
}

void PmidNodeService::ValidateGetSender(const nfs::Message& message) const {
  if (!SenderInGroupForMetadata(message, routing_))
    ThrowError(VaultErrors::permission_denied);

  if (!FromDataManager(message) || !ForThisPersona(message))
    ThrowError(CommonErrors::invalid_parameter);
}

void PmidNodeService::ValidateDeleteSender(const nfs::Message& message) const {
  if (!SenderIsConnectedVault(message, routing_))
    ThrowError(VaultErrors::permission_denied);

  if (!FromPmidManager(message) || !ForThisPersona(message))
    ThrowError(CommonErrors::invalid_parameter);
}

}  // namespace vault

}  // namespace maidsafe
