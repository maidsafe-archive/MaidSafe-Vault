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

#ifndef MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_HANDLER_H_
#define MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_HANDLER_H_

#include <cstdint>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/on_scope_exit.h"
#include "maidsafe/common/types.h"

#include "maidsafe/vault/metadata_manager/metadata_value.h"
#include "maidsafe/vault/metadata_manager/metadata_manager.h"
#include "maidsafe/vault/metadata_manager/metadata_handler.h"
#include "maidsafe/vault/metadata_manager/metadata_helpers.h"
#include "maidsafe/vault/manager_db.h"
#include "maidsafe/vault/metadata_manager/metadata.pb.h"
#include "maidsafe/vault/metadata_manager/metadata_merge_policy.h"
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

  // Returns a pair of - is duplicate data and its cost.
  // Checks for duplication of unique data (throws)
  template<typename Data>
  std::pair<bool, int32_t> CheckPut(const typename Data::name_type& data_name, int32_t data_size);

  void AddLocalUnresolvedEntry(const MetadataUnresolvedEntry& unresolved_entry);

  // Sync operations
  std::vector<DataNameVariant> GetRecordNames() const;
  serialised_record_type GetSerialisedRecord(const DataNameVariant& data_name);
  template <typename Data>
  NonEmptyString GetSyncData(const typename Data::name_type& data_name);
  void ApplySyncData(const NonEmptyString& serialised_unresolved_entries);
  void ReplaceNodeInSyncList(const DataNameVariant& record_name,
                             const NodeId& old_node,
                             const NodeId& new_node);
  template<typename Data>
  void IncrementSyncAttempts(const typename Data::name_type& data_name);


  template<typename Data>
  friend class MetadataHandlerTypedTest;

 private:
  const boost::filesystem::path kMetadataRoot_;
  std::unique_ptr<ManagerDb<MetadataManager>> metadata_db_;
  const NodeId kThisNodeId_;
  mutable std::mutex mutex_;
  Sync<MetadataMergePolicy> sync_;
  static const size_t kSyncTriggerCount_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/metadata_manager/metadata_handler-inl.h"

#endif  // MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_HANDLER_H_
