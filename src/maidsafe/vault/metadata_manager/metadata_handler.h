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

#include "maidsafe/vault/metadata_manager/metadata_pb.h"
#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

class MetadataHandler {
 public:
  explicit MetadataHandler(const boost::filesystem::path& vault_root_dir);

  // This increments the subscribers count, or adds a new element if it doesn't exist.
  template<typename Data>
  void IncrementSubscribers(const typename Data::name_type& data_name, int32_t data_size);
  // This decrements the subscribers count.  If it hits 0, the element is removed.
  template<typename Data>
  void DecrementSubscribers(const typename Data::name_type& data_name);

  // This is used when synchronising with other MMs.  It simply adds or replaces any existing
  // element of the same type and name.
  void PutMetadata(const protobuf::Metadata& proto_metadata);
  // This is used when synchronising with other MMs.  If this node sends a sync (group message) for
  // this element, and doesn't receive its own request, it's no longer responsible for this element.
  template<typename Data>
  void DeleteMetadata(const typename Data::name_type& data_name);

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
  void CheckMetadataExists(const typename Data::name_type& data_name) const;

 private:
  template<typename Data>
  struct Metadata {
    // This constructor reads the existing element or creates a new one if it doesn't already exist.
    Metadata(const typename Data::name_type& data_name,
             const boost::filesystem::path& root,
             int32_t data_size);
    // This constructor reads the existing element or throws if it doesn't already exist.
    Metadata(const typename Data::name_type& data_name, const boost::filesystem::path& root);
    // Should only be called once.
    void SaveChanges();

    protobuf::Metadata content;
    const boost::filesystem::path kPath;
    on_scope_exit strong_guarantee;

   private:
    Metadata();
    Metadata(const Metadata&);
    Metadata& operator=(const Metadata&);
    Metadata(Metadata&&);
    Metadata& operator=(Metadata&&);
  };

  const boost::filesystem::path kMetadataRoot_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/metadata_manager/metadata_handler-inl.h"

#endif  // MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_HANDLER_H_
