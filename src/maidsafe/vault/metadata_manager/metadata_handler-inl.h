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

#ifndef MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_HANDLER_INL_H_
#define MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_HANDLER_INL_H_

#include <set>
#include <string>
#include <vector>

#include "boost/filesystem/operations.hpp"

#include "maidsafe/common/error.h"
#include "maidsafe/common/utils.h"

namespace fs = boost::filesystem;

namespace maidsafe {

namespace vault {

namespace detail {

boost::filesystem::path GetPath(const std::string& data_name,
                                int32_t data_type_enum_value,
                                const boost::filesystem::path& root);

template<typename Data>
boost::filesystem::path GetPath(const typename Data::name_type& data_name,
                                const boost::filesystem::path& root) {
  return GetPath(data_name->string(), static_cast<int>(Data::type_enum_value()), root);
}

//std::set<std::string> OnlinesToSet(const protobuf::Metadata& content);
//std::set<std::string> OfflinesToSet(const protobuf::Metadata& content);
//void OnlinesToProtobuf(const std::set<std::string>& onlines, protobuf::Metadata& content);
//void OfflinesToProtobuf(const std::set<std::string>& offlines, protobuf::Metadata& content);

}  // namespace detail

template<typename Data>
void MetadataHandler::IncrementSubscribers(const typename Data::name_type& /*data_name*/,
                                           int32_t /*data_size*/) {
//  Metadata<Data> metadata(data_name, metadata_db_.get(), data_size);
//  MetadataValueDelta metadata_value_delta;
//  metadata_value_delta.data_size = metadata.value_.data_size;
}

//template<typename Data>
//void MetadataHandler::IncrementSubscribers(const typename Data::name_type& data_name,
//                                           int32_t data_size) {
//  Metadata<Data> metadata(data_name, metadata_db_.get(), data_size);
//  ++metadata.value_.subscribers;
//  metadata.SaveChanges(metadata_db_.get());
//}

template<typename Data>
void MetadataHandler::DecrementSubscribers(const typename Data::name_type& /*data_name*/) {
//  Metadata<Data> metadata(data_name, metadata_db_.get());
//  --metadata.value_.subscribers;
//  metadata.SaveChanges(metadata_db_.get());
}

template<typename Data>
void MetadataHandler::DeleteMetadata(const typename Data::name_type& /*data_name*/) {
//  Metadata<Data> metadata(data_name, kMetadataRoot_);
//  metadata.value_.subscribers = 0;
//  metadata.SaveChanges();
}

template<typename Data>
void MetadataHandler::MarkNodeDown(const typename Data::name_type& /*data_name*/,
                                   const PmidName& /*pmid_name*/,
                                   int& /*remaining_online_holders*/) {
//  Metadata<Data> metadata(data_name, kMetadataRoot_);
//  metadata.value_.online_pmid_name.erase(pmid_name);
//  metadata.value_.offline_pmid_name.insert(pmid_name);
//  remaining_online_holders = metadata.value_.online_pmid_name.size();
//  metadata.SaveChanges();
}

template<typename Data>
void MetadataHandler::MarkNodeUp(const typename Data::name_type& /*data_name*/,
                                 const PmidName& /*pmid_name*/) {
//  Metadata<Data> metadata(data_name, kMetadataRoot_);
//  metadata.value_.online_pmid_name.insert(pmid_name);
//  metadata.value_.offline_pmid_name.erase(pmid_name);
//  metadata.SaveChanges();
}

template<typename Data>
void MetadataHandler::AddDataHolder(const typename Data::name_type& /*data_name*/,
                                    const PmidName& /*online_pmid_name*/) {
//  Metadata<Data> metadata(data_name, kMetadataRoot_);
//  metadata.value_.online_pmid_name.insert(online_pmid_name);
//  metadata.SaveChanges();
}

template<typename Data>
void MetadataHandler::RemoveDataHolder(const typename Data::name_type& /*data_name*/,
                                       const PmidName& /*pmid_name*/) {
//  Metadata<Data> metadata(data_name, kMetadataRoot_);
//  metadata.value_.online_pmid_name.erase(pmid_name);
//  metadata.value_.offline_pmid_name.erase(pmid_name);
//  metadata.SaveChanges();
}

template<typename Data>
std::vector<PmidName> MetadataHandler::GetOnlineDataHolders(
    const typename Data::name_type& /*data_name*/) const {
//  Metadata<Data> metadata(data_name, metadata_db_.get());
//  std::vector<PmidName> onlines(metadata.value_.online_pmid_name.begin(),
//                                metadata.value_.online_pmid_name.end());
//  metadata.strong_guarantee_.Release();
//  return onlines;
  return std::vector<PmidName>();
}

template<typename Data>
bool MetadataHandler::CheckMetadataExists(const typename Data::name_type& /*data_name*/) const {
//  try {
//    Metadata<Data> metadata(data_name, metadata_db_.get());
//    metadata.strong_guarantee_.Release();
//  } catch (const maidsafe_error& error) {
//    return false;
//  }
  return true;
}

template<typename Data>
int32_t MetadataHandler::CheckPut(const typename Data::name_type& /*data_name*/, int32_t /*data_size*/) {
  return 0;
}


template<typename Data>
void MetadataHandler::ApplySyncData(const typename Data::name_type& /*data_name*/,
                                    const NonEmptyString& /*serialised_unresolved_entries*/) {

}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_HANDLER_INL_H_
