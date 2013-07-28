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
#include "maidsafe/nfs/client_utils.h"

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
      miscellaneous_policy(routing_, pmid),
      nfs_(routing_, pmid) {
//  nfs_.GetElementList();  // TODO (Fraser) BEFORE_RELEASE Implementation needed
}

void PmidNodeService::SendAccountRequest() {
  auto response_vector(std::make_shared<std::vector<protobuf::PmidAccountResponse>>());
  auto mutex(std::make_shared<std::mutex>());
  auto done(std::make_shared<bool>(false));
  miscellaneous_policy.RequestPmidNodeAccount(PmidName(Identity(routing_.kNodeId().string())),
      [done, response_vector, mutex, this](std::string serialised_reply) {
        std::map<DataNameVariant, u_int16_t> expected_chunks;
        {
          std::lock_guard<std::mutex> lock(*mutex);
          if (*done)
            return;
          nfs::Reply reply((nfs::Reply::serialised_type(NonEmptyString(serialised_reply))));
          if (!reply.IsSuccess()) {
            LOG(kWarning) << "Failure in PmidAccount response";
            return;
          }
          protobuf::PmidAccountResponse pmid_account_response;
          if (pmid_account_response.ParseFromString(reply.data().string())) {
            response_vector->push_back(pmid_account_response);
          } else {
            LOG(kWarning) << "Failed to parse PmidAccount response";
            return;
          }
          uint16_t total_replies(response_vector->size()), total_valid_replies(0);
          assert((total_replies <= routing::Parameters::node_group_size) &&
                 "Invalid number of account transfer");
          if (total_replies <= routing::Parameters::node_group_size / 2) {
            return;
          } else {
            total_valid_replies = this->TotalValidPmidAccountReplies(response_vector);
            if ((total_replies >= (routing::Parameters::node_group_size / 2 + 1)) &&
                 total_valid_replies >= routing::Parameters::node_group_size / 2) {
              ApplyAccountTransfer(total_replies, total_valid_replies, expected_chunks);
              *done = true;
            }
          }
        }
        if (*done)
          this->UpdateLocalStorage(expected_chunks);
      });
}

void PmidNodeService::ApplyAccountTransfer(const size_t& total_pmidmgrs,
                                           const size_t& pmidmgrs_with_account,
                                           std::map<DataNameVariant, uint16_t>& chunks) {
  struct ChunkInfo {
    ChunkInfo(const DataNameVariant& file_name_in, const uint64_t& size_in) :
        file_name(file_name_in), size(size_in) {}
    DataNameVariant file_name;
    uint64_t size;
  };

  struct ChunkInfoComparison {
    bool operator() (const ChunkInfo& lhs, const ChunkInfo& rhs) const {
      return lhs.file_name < rhs.file_name;
    }
  };

  std::map<ChunkInfo, uint16_t, ChunkInfoComparison> expected_chunks;
  protobuf::PmidAccountResponse pmid_account_response;
  protobuf::PmidAccountDetails pmid_account_details;

  for (auto pending_request : accumulator_.pending_requests_) {
    if (static_cast<nfs::MessageAction>(pending_request.msg.data().action) ==
            nfs::MessageAction::kAccountTransfer) {
      pmid_account_response.ParseFromString(pending_request.msg.data().content.string());
      if (pmid_account_response.status() == static_cast<int>(CommonErrors::success)) {
        pmid_account_details.ParseFromString(
            pmid_account_response.pmid_account().serialised_account_details());
        for (int index(0); index < pmid_account_details.db_entry_size(); ++index) {
          ChunkInfo chunk_info(
              ImmutableData::name_type(Identity(pmid_account_details.db_entry(index).name())),
              pmid_account_details.db_entry(index).value().size());
          expected_chunks[chunk_info]++;
        }
      }
    }
  }
  for (auto iter(expected_chunks.begin()); iter != expected_chunks.end(); ++iter) {
    if ((iter->second >= routing::Parameters::node_group_size / 2 + 1) ||
        ((iter->second == routing::Parameters::node_group_size / 2)
         && (total_pmidmgrs > pmidmgrs_with_account))) {
      std::pair<DataNameVariant, int16_t> pair(iter->first.file_name, iter->second);
      chunks.insert(pair);
    }
  }
}

void PmidNodeService::UpdateLocalStorage(
    const std::map<DataNameVariant, uint16_t>& expected_files) {
  std::vector<DataNameVariant> existing_files(StoredFileNames());
  std::vector<DataNameVariant> to_be_deleted, to_be_retrieved;
  for (auto file_name : existing_files) {
    if (std::find_if(expected_files.begin(),
                     expected_files.end(),
                     [&file_name](const std::pair<DataNameVariant, bool>& expected) {
                       return expected.first == file_name;
                     }) == expected_files.end()) {
      to_be_deleted.push_back(file_name);
    }
  }
  for (auto iter(expected_files.begin()); iter != expected_files.end(); ++iter) {
    if ((std::find_if(existing_files.begin(),
                     existing_files.end(),
                     [&](const DataNameVariant& existing) {
                       return existing == iter->first;
                     }) == existing_files.end()) &&
        (iter->second >= routing::Parameters::node_group_size / 2 + 1)) {
      to_be_retrieved.push_back(iter->first);
    }
  }
  ApplyUpdateLocalStorage(to_be_deleted, to_be_retrieved);
}

void PmidNodeService::ApplyUpdateLocalStorage(const std::vector<DataNameVariant>& to_be_deleted,
                                              const std::vector<DataNameVariant>& to_be_retrieved) {
  for (auto file : to_be_deleted) {
    try {
      permanent_data_store_.Delete(file);
    }
    catch(const maidsafe_error& error) {
      LOG(kWarning) << "Error in deletion: " << error.code() << " - " << error.what();
    }
  }

  std::vector<std::future<std::unique_ptr<ImmutableData>>> futures;
  for (auto file : to_be_retrieved)
    futures.push_back(RetrieveFileFromNetwork(file));

  for (auto iter(futures.begin()); iter != futures.end(); ++iter) {
    try {
      iter->wait();
    }
    catch(const maidsafe_error& error) {
      LOG(kWarning) << "Error in retreivel: " << error.code() << " - " << error.what();
    }
  }
}

std::future<std::unique_ptr<ImmutableData>>
PmidNodeService::RetrieveFileFromNetwork(const DataNameVariant& file_id) {
  auto result(boost::apply_visitor(GetTagValueAndIdentityVisitor(), file_id));
  ImmutableData::name_type name(result.second);
  auto opt_mutex(std::make_shared<std::mutex>()) ;
  auto get_op(std::make_shared<nfs::detail::GetOp<ImmutableData>>());
  nfs_.Get<ImmutableData>(ImmutableData::name_type(Identity(result.second)),
                          [this, get_op, name, &opt_mutex](std::string serialised_reply) {
    try {
      nfs::Reply reply((nfs::Reply::serialised_type(NonEmptyString(serialised_reply))));
      if (!reply.IsSuccess())
        throw reply.error();
      {
        std::lock_guard<std::mutex> lock(*opt_mutex);
        if (!get_op->IsPromiseSet()) {
          ImmutableData data(name, (ImmutableData::serialised_type(reply.data())));
          permanent_data_store_.Put(data.name(), reply.data());
          get_op->SetPromiseValue(std::move(data));
        }
      }
    }
    catch(const maidsafe_error& error) {
      LOG(kWarning) << "Error in retreivel: " << error.code() << " - " << error.what();
      get_op->HandleFailure(error);
    }
    catch(const std::exception& e) {
      LOG(kWarning) << "Error in retreivel: " << e.what();
      get_op->HandleFailure(MakeError(CommonErrors::unknown));
    }
  });
  return get_op->GetFutureFromPromise();
}

std::vector<DataNameVariant> PmidNodeService::StoredFileNames() {
  std::vector<DataNameVariant> file_ids;
  fs::directory_iterator end_iter;
  auto root_path(permanent_data_store_.GetDiskPath());

  if (fs::exists(root_path) && fs::is_directory(root_path)) {
    for(fs::directory_iterator dir_iter(root_path); dir_iter != end_iter; ++dir_iter)
      if (fs::is_regular_file(dir_iter->status()))
        file_ids.push_back(PmidName(Identity(dir_iter->path().string())));
  }
  return file_ids;
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

uint16_t PmidNodeService::TotalValidPmidAccountReplies(
    std::shared_ptr<std::vector<protobuf::PmidAccountResponse>> response_vector) const {
  return std::count_if(response_vector->begin(),
                       response_vector->end(),
                       [](const protobuf::PmidAccountResponse& response) {
                         return response.status() == static_cast<int>(CommonErrors::success);
                       });
}

}  // namespace vault

}  // namespace maidsafe
