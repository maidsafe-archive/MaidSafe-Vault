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

#include "boost/filesystem/operations.hpp"

#include "maidsafe/common/error.h"
#include "maidsafe/common/utils.h"


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

std::set<std::string> OnlinesToSet(protobuf::Metadata& content);
std::set<std::string> OfflinesToSet(protobuf::Metadata& content);
void OnlinesToProtobuf(const std::set<std::string>& onlines, protobuf::Metadata& content);
void OfflinesToProtobuf(const std::set<std::string>& offlines, protobuf::Metadata& content);

}  // namespace detail


template<typename Data>
MetadataHandler::Metadata<Data>::Metadata(const typename Data::name_type& data_name,
                                          const boost::filesystem::path& root,
                                          int32_t data_size)
    : content(),
      kPath(detail::GetPath<Data>(data_name, root)),
      strong_guarantee(on_scope_exit::ExitAction()) {
  if (boost::filesystem::exists(kPath)) {
    auto serialised_content(ReadFile(kPath));
    if (!content.ParseFromString(serialised_content.string()) ||
        content.type() != static_cast<int>(Data::type_enum_value()) ||
        content.name() != data_name->string() ||
        content.size() != data_size ||
        content.subscribers() < 1) {
      LOG(kError) << "Failed to read or parse metadata file " << kPath;
      ThrowError(CommonErrors::parsing_error);
    }
  } else {
    content.set_type(static_cast<int>(Data::type_enum_value()));
    content.set_name(data_name->string());
    content.set_size(data_size);
  }
  strong_guarantee.SetAction(on_scope_exit::RevertValue(content));
}

template<typename Data>
MetadataHandler::Metadata<Data>::Metadata(const typename Data::name_type& data_name,
                                          const boost::filesystem::path& root)
    : content(),
      kPath(detail::GetPath<Data>(data_name, root)),
      strong_guarantee(on_scope_exit::ExitAction()) {
  if (boost::filesystem::exists(kPath)) {
    auto serialised_content(ReadFile(kPath));
    if (!content.ParseFromString(serialised_content.string()) ||
        content.type() != static_cast<int>(Data::type_enum_value()) ||
        content.name() != data_name->string() ||
        content.subscribers() < 1) {
      LOG(kError) << "Failed to read or parse metadata file " << kPath;
      ThrowError(CommonErrors::parsing_error);
    }
  } else {
    LOG(kError) << "Failed to find metadata file " << kPath;
    ThrowError(CommonErrors::no_such_element);
  }
  strong_guarantee.SetAction(on_scope_exit::RevertValue(content));
}

template<typename Data>
void MetadataHandler::IncrementSubscribers(const typename Data::name_type& data_name,
                                           int32_t data_size) {
  Metadata<Data> metadata(data_name, kMetadataRoot_, data_size);
  metadata.content.set_subscribers(metadata.content.subscribers() + 1);
  metadata.SaveChanges();
}

template<typename Data>
void MetadataHandler::DecrementSubscribers(const typename Data::name_type& data_name) {
  Metadata<Data> metadata(data_name, kMetadataRoot_);
  metadata.content.set_subscribers(metadata.content.subscribers() - 1);
  metadata.SaveChanges();
}

template<typename Data>
void MetadataHandler::DeleteMetadata(const typename Data::name_type& data_name) {
  Metadata<Data> metadata(data_name, kMetadataRoot_);
  metadata.content.set_subscribers(0);
  metadata.SaveChanges();
}

template<typename Data>
void MetadataHandler::MarkNodeDown(const typename Data::name_type& data_name,
                                   const PmidName& pmid_name,
                                   int& remaining_online_holders) {
  Metadata<Data> metadata(data_name, kMetadataRoot_);

  auto onlines(detail::OnlinesToSet(metadata.content));
  onlines.erase(pmid_name->string());
  detail::OnlinesToProtobuf(onlines, metadata.content);

  auto offlines(detail::OfflinesToSet(metadata.content));
  offlines.insert(pmid_name->string());
  detail::OfflinesToProtobuf(offlines, metadata.content);

  remaining_online_holders = metadata.content.online_pmid_name_size();
  metadata.SaveChanges();
}

template<typename Data>
void MetadataHandler::MarkNodeUp(const typename Data::name_type& data_name,
                                 const PmidName& pmid_name) {
  Metadata<Data> metadata(data_name, kMetadataRoot_);

  auto onlines(detail::OnlinesToSet(metadata.content));
  onlines.insert(pmid_name->string());
  detail::OnlinesToProtobuf(onlines, metadata.content);

  auto offlines(detail::OfflinesToSet(metadata.content));
  offlines.erase(pmid_name->string());
  detail::OfflinesToProtobuf(offlines, metadata.content);

  metadata.SaveChanges();
}

template<typename Data>
void MetadataHandler::AddDataHolder(const typename Data::name_type& data_name,
                                    const PmidName& online_pmid_name) {
  Metadata<Data> metadata(data_name, kMetadataRoot_);
  metadata.content.add_online_pmid_name(online_pmid_name->string());
  metadata.SaveChanges();
}

template<typename Data>
void MetadataHandler::RemoveDataHolder(const typename Data::name_type& data_name,
                                       const PmidName& pmid_name) {
  Metadata<Data> metadata(data_name, kMetadataRoot_);

  auto onlines(detail::OnlinesToSet(metadata.content));
  onlines.erase(pmid_name->string());
  detail::OnlinesToProtobuf(onlines, metadata.content);

  auto offlines(detail::OfflinesToSet(metadata.content));
  offlines.erase(pmid_name->string());
  detail::OnlinesToProtobuf(onlines, metadata.content);

  metadata.SaveChanges();
}

template<typename Data>
std::vector<PmidName> MetadataHandler::GetOnlineDataHolders(
    const typename Data::name_type& data_name) const {
  Metadata<Data> metadata(data_name, kMetadataRoot_);
  std::vector<PmidName> onlines;
  for (int i(0); i != metadata.content.online_pmid_name_size(); ++i)
    onlines.push_back(PmidName(Identity(metadata.content.online_pmid_name(i))));
  metadata.strong_guarantee.Release();
  return onlines;
}

template<typename Data>
void MetadataHandler::CheckMetadataExists(const typename Data::name_type& data_name) const {
  Metadata<Data> metadata(data_name, kMetadataRoot_);
  metadata.strong_guarantee.Release();
}

template<typename Data>
void MetadataHandler::Metadata<Data>::SaveChanges() {
  if (content.subscribers() < 1) {
    if (!boost::filesystem::remove(kPath)) {
      LOG(kError) << "Failed to remove metadata file " << kPath;
      ThrowError(CommonErrors::filesystem_io_error);
    }
  } else {
    std::string serialised_content(content.SerializeAsString());
    if (serialised_content.empty()) {
      LOG(kError) << "Failed to serialise metadata file " << kPath;
      ThrowError(CommonErrors::serialisation_error);
    }
    if (!WriteFile(kPath, serialised_content)) {
      LOG(kError) << "Failed to write metadata file " << kPath;
      ThrowError(CommonErrors::filesystem_io_error);
    }
  }
  strong_guarantee.Release();
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_HANDLER_INL_H_
