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

#ifndef MAIDSAFE_VAULT_PMID_NODE_SERVICE_H_
#define MAIDSAFE_VAULT_PMID_NODE_SERVICE_H_

#include <mutex>
#include <type_traits>
#include <set>
#include <vector>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "maidsafe/common/types.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/data_store/data_store.h"
#include "maidsafe/data_store/memory_buffer.h"
#include "maidsafe/data_store/permanent_store.h"
#include "maidsafe/data_types/data_type_values.h"
#include "maidsafe/nfs/message_types.h"
#include "maidsafe/vault/message_types.h"


#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/types.h"
//#include "maidsafe/vault/post_policies.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.pb.h"


namespace maidsafe {

namespace vault {

namespace protobuf {
  class PmidAccountResponse;
}  // namespace protobuf

namespace test {

template<typename Data>
class DataHolderTest;

}  // namespace test


class PmidNodeService {
 public:

  enum : uint32_t { kPutRequestsRequired = 3, kDeleteRequestsRequired = 3 };

  PmidNodeService(const passport::Pmid& pmid,
                  routing::Routing& routing,
                  const boost::filesystem::path& vault_root_dir);

  template<typename T>
  void HandleMessage(const T& message,
                     const typename T::Sender& sender,
                     const typename T::Receiver& receiver) {
    T::invalid_message_type_passed::should_be_one_of_the_specialisations_defined_below;
  }

  template<typename T>
  bool GetFromCache(const T& /*message*/,
                    const typename T::Sender& /*sender*/,
                    const typename T::Receiver& /*receiver*/) {
    T::invalid_message_type_passed::should_be_one_of_the_specialisations_defined_below;
  }

  template<typename T>
  void StoreInCache(const T& /*message*/,
                    const typename T::Sender& /*sender*/,
                    const typename T::Receiver& /*receiver*/) {
    T::invalid_message_type_passed::should_be_one_of_the_specialisations_defined_below;
  }

  template<typename Data>
  friend class test::DataHolderTest;

 private:
  typedef std::true_type IsCacheable, IsLongTermCacheable;
  typedef std::false_type IsNotCacheable, IsShortTermCacheable;

  template<typename Data>
  void HandlePutMessage(const nfs::PutRequestFromPmidManagerToPmidNode& message);
  template<typename Data>
  void HandleGetMessage(const nfs::GetRequestFromDataManagerToPmidNode& message);
  template<typename Data>
  void HandleDeleteMessage(const nfs::DeleteRequestFromPmidManagerToPmidNode);

  void SendAccountRequest();

  // populates chunks map
  void ApplyAccountTransfer(std::make_shared<std::vector<protobuf::PmidAccountResponse>> responses,
                            const size_t& total_pmidmgrs,
                            const size_t& pmidmagsr_with_account,
                            std::map<DataNameVariant, uint16_t>& chunks);
  void UpdateLocalStorage(const std::map<DataNameVariant, uint16_t>& expected_files);
  void ApplyUpdateLocalStorage(const std::vector<DataNameVariant>& to_be_deleted,
                               const std::vector<DataNameVariant>& to_be_retrieved);
  std::vector<DataNameVariant> StoredFileNames();
  uint16_t TotalValidPmidAccountReplies(
      std::shared_ptr<std::vector<protobuf::PmidAccountResponse>> response_vector) const;

  std::future<std::unique_ptr<ImmutableData>>
  RetrieveFileFromNetwork(const DataNameVariant& file_id);

  void ValidatePutSender(const nfs::Message& message) const;
  void ValidateGetSender(const nfs::Message& message) const;
  void ValidateDeleteSender(const nfs::Message& message) const;

  template<typename T>
  bool GetFromCache(const T& message,
                    const typename T::Sender& sender,
                    const typename T::Receiver& receiver,
                    IsCacheable);

  template<typename T>
  bool GetFromCache(const T& message,
                    const typename T::Sender& sender,
                    const typename T::Receiver& receiver,
                    IsNotCacheable);

  template<typename T>
  bool CacheGet(const T& message,
                const typename T::Sender& sender,
                const typename T::Receiver& receiver,
                IsShortTermCacheable);

  template<typename T>
  bool CacheGet(const T& message,
                const typename T::Sender& sender,
                const typename T::Receiver& receiver,
                IsLongTermCacheable);

  template<typename T>
  void StoreInCache(const T& message,
                    const typename T::Sender& sender,
                    const typename T::Receiver& receiver,
                    IsCacheable);

  template<typename T>
  void StoreInCache(const T& message,
                    const typename T::Sender& sender,
                    const typename T::Receiver& receiver,
                    IsNotCacheable);

  template<typename T>
  void CacheStore(const T& message, IsShortTermCacheable);

  template<typename T>
  void CacheStore(const T& message, IsLongTermCacheable);

  boost::filesystem::space_info space_info_;
  DiskUsage disk_total_;
  DiskUsage permanent_size_;
  DiskUsage cache_size_;
  data_store::PermanentStore permanent_data_store_;
  data_store::DataStore<data_store::DataBuffer> cache_data_store_;
  data_store::MemoryBuffer mem_only_cache_;
  routing::Routing& routing_;
  std::mutex accumulator_mutex_;
  Accumulator<PmidNodeServiceMessages> accumulator_;
};

template<>
void PmidNodeService::HandleMessage<nfs::GetRequestFromDataManagerToPmidNode>(
    const nfs::GetRequestFromDataManagerToPmidNode& message,
    const typename nfs::GetRequestFromDataManagerToPmidNode::Sender& sender,
    const typename nfs::GetRequestFromDataManagerToPmidNode::Receiver& receiver);

template<>
void PmidNodeService::HandleMessage<nfs::DeleteRequestFromPmidManagerToPmidNode>(
    const nfs::DeleteRequestFromPmidManagerToPmidNode& message,
    const typename nfs::DeleteRequestFromPmidManagerToPmidNode::Sender& sender,
    const typename nfs::DeleteRequestFromPmidManagerToPmidNode::Receiver& receiver);

template<>
void PmidNodeService::HandleMessage<nfs::PutRequestFromPmidManagerToPmidNode>(
    const nfs::PutRequestFromPmidManagerToPmidNode& message,
    const typename nfs::PutRequestFromPmidManagerToPmidNode::Sender& sender,
    const typename nfs::PutRequestFromPmidManagerToPmidNode::Receiver& receiver);

template<>
bool PmidNodeService::GetFromCache<nfs::GetRequestFromMaidNodeToDataManager>(
    const nfs::GetRequestFromMaidNodeToDataManager& message,
    const typename nfs::GetRequestFromMaidNodeToDataManager::Sender& sender,
    const typename nfs::GetRequestFromMaidNodeToDataManager::Receiver& receiver);

template<>
bool PmidNodeService::GetFromCache<nfs::GetRequestFromPmidNodeToDataManager>(
    const nfs::GetRequestFromPmidNodeToDataManager& message,
    const typename nfs::GetRequestFromPmidNodeToDataManager::Sender& sender,
    const typename nfs::GetRequestFromPmidNodeToDataManager::Receiver& receiver);

template<>
void PmidNodeService::StoreInCache<nfs::GetResponseFromDataManagerToMaidNode>(
    const nfs::GetResponseFromDataManagerToMaidNode& message,
    const typename nfs::GetResponseFromDataManagerToMaidNode::Sender& sender,
    const typename nfs::GetResponseFromDataManagerToMaidNode::Receiver& receiver);

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/pmid_node/service-inl.h"

#endif  // MAIDSAFE_VAULT_PMID_NODE_SERVICE_H_
