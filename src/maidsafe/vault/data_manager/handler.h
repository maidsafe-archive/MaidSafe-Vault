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

#ifndef MAIDSAFE_VAULT_DATA_MANAGER_HANDLER_H_
#define MAIDSAFE_VAULT_DATA_MANAGER_HANDLER_H_

#include <cstdint>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/on_scope_exit.h"
#include "maidsafe/common/types.h"

#include "maidsafe/vault/data_manager/value.h"
#include "maidsafe/vault/data_manager/data_manager.h"
#include "maidsafe/vault/data_manager/helpers.h"
#include "maidsafe/vault/manager_db.h"
#include "maidsafe/vault/data_manager/data_manager.pb.h"
#include "maidsafe/vault/data_manager/merge_policy.h"
#include "maidsafe/vault/sync.h"
#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

namespace test {

template<typename Data>
class MetadataHandlerTypedTest;

}  // namespace test

class MetadataHandler {
 public:
  typedef TaggedValue<NonEmptyString, struct SerialisedMetadataValueTag>
    serialised_record_type;
  MetadataHandler(const boost::filesystem::path& vault_root_dir, const NodeId& this_node_id);

  // This increments the subscribers count, or adds a new element if it doesn't exist.
  template<typename Data>
  void IncrementSubscribers(const typename Data::name_type& data_name, int32_t data_size);
  // This decrements the subscribers count.  If it hits 0, the element is removed.
  template<typename Data>
  void DecrementSubscribers(const typename Data::name_type& data_name);

  // This is used when synchronising with other MMs.  It simply adds or replaces any existing
  // element of the same type and name.
  //  void PutMetadata(const protobuf::Metadata& proto_metadata);
  // This is used when synchronising with other MMs.  If this node sends a sync (group message) for
  // this element, and doesn't receive its own request, it's no longer responsible for this element.
  template<typename Data>
  void DeleteMetadata(const typename Data::name_type& data_name);

  void DeleteRecord(const DataNameVariant& record_name);

  template<typename Data>
  void MarkNodeDown(const typename Data::name_type& data_name,
                    const PmidName& pmid_name,
                    int& remaining_online_holders);
  template<typename Data>
  void MarkNodeUp(const typename Data::name_type& data_name, const PmidName& pmid_name);

  // The data holder is assumed to be online
  template<typename Data>
  void AddDataHolder(const typename Data::name_type& data_name, const PmidName& online_pmid_name);
  // The data holder could be online or offline
  template<typename Data>
  void RemoveDataHolder(const typename Data::name_type& data_name, const PmidName& pmid_name);

  template<typename Data>
  std::vector<PmidName> GetOnlineDataHolders(const typename Data::name_type& data_name) const;

  template<typename Data>
  bool CheckMetadataExists(const typename Data::name_type& data_name) const;

  // Returns a pair of - is already stored and its cost.
  // Checks for duplication of unique data (throws)
  template<typename Data>
  std::pair<bool, int32_t> CheckPut(const typename Data::name_type& data_name, int32_t data_size);

  void AddLocalUnresolvedEntry(const DataManagerUnresolvedEntry& unresolved_entry);

  // Sync operations
  std::vector<DataManager::RecordName> GetRecordNames() const;
  serialised_record_type GetSerialisedRecord(const DataNameVariant& data_name);
  template <typename Data>
  NonEmptyString GetSyncData(const typename Data::name_type& data_name);
  std::vector<DataManagerUnresolvedEntry> GetSyncData();
  void ApplySyncData(const NonEmptyString& serialised_unresolved_entry);
  void ApplyRecordTransfer(const NonEmptyString& serialised_unresolved_entries);
  void ReplaceNodeInSyncList(const DataNameVariant& record_name,
                             const NodeId& old_node,
                             const NodeId& new_node);
  template<typename Data>
  void IncrementSyncAttempts(const typename Data::name_type& data_name);


  template<typename Data>
  friend class MetadataHandlerTypedTest;

 private:
  const boost::filesystem::path kMetadataRoot_;
  std::unique_ptr<ManagerDb<DataManager>> metadata_db_;
  const NodeId kThisNodeId_;
  mutable std::mutex mutex_;
  Sync<MetadataMergePolicy> sync_;
  static const size_t kSyncTriggerCount_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/data_manager/handler-inl.h"

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_HANDLER_H_
