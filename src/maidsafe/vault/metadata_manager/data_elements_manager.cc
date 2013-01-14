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

#include "maidsafe/vault/metadata_manager/data_elements_manager.h"

#include "maidsafe/vault/metadata_manager/metadata_pb.h"

#include "boost/filesystem/operations.hpp"
#include "maidsafe/common/utils.h"


namespace maidsafe {

namespace vault {

namespace {

bool RemovePmidFromOnlineList(const std::string& pmid, protobuf::MetadataElement& element) {
  std::vector<std::string> pmid_names;
  bool found(false);
  for (int n(0); n < element.element_size(); ++n) {
    if (element.online_pmid_name(n) == pmid)
      found = true;
    else
      pmid_names.push_back(element.online_pmid_name(n));
  }

  if (found) {
    element.clear_online_pmid_name();
    for (auto& pmid_name : pmid_names)
      element.add_online_pmid_name(pmid_name);
  }

  return found;
}

bool RemovePmidFromOfflineList(const std::string& pmid, protobuf::MetadataElement& element) {
  std::vector<std::string> pmid_names;
  bool found(false);
  for (int n(0); n < element.element_size(); ++n) {
    if (element.offline_pmid_name(n) == pmid)
      found = true;
    else
      pmid_names.push_back(element.offline_pmid_name(n));
  }

  if (found) {
    element.clear_offline_pmid_name();
    for (auto& pmid_name : pmid_names)
      element.add_offline_pmid_name(pmid_name);
  }

  return found;
}

}  // namespace

const boost::filesystem::path kVaultDirectory("meta_data_manager");

DataElementsManager::DataElementsManager(const boost::filesystem::path& vault_root_dir)
    : vault_metadata_dir_(vault_root_dir / kVaultDirectory) {
  if (!boost::filesystem::exists(vault_metadata_dir_))
    boost::filesystem::create_directories(vault_metadata_dir_);
}

void DataElementsManager::AddDataElement(const Identity& data_name,
                                         int32_t element_size,
                                         const PmidName& online_pmid_name,
                                         const PmidName& offline_pmid_name) {
  protobuf::MetadataElement element;
  try {
    CheckDataElementExists(data_name);
    // Increase counter
    ReadAndParseElement(data_name, element);
    int64_t number_stored(element.number_stored());
    element.set_number_stored(number_stored + 1);
  }
  catch(...) {
    // Add new entry
    element.set_data_name(data_name.string());
    element.set_element_size(element_size);
    element.set_number_stored(1);
    element.add_online_pmid_name(online_pmid_name->string());
    element.add_offline_pmid_name(offline_pmid_name->string());
  }

  SerialiseAndSaveElement(element);
}

void DataElementsManager::RemoveDataElement(const Identity& data_name) {
  CheckDataElementExists(data_name);
  boost::filesystem::remove(vault_metadata_dir_ / EncodeToBase64(data_name));
}

void DataElementsManager::MoveNodeToOffline(const Identity& data_name,
                                            const PmidName& pmid_name,
                                            int64_t& holders) {
  CheckDataElementExists(data_name);
  protobuf::MetadataElement element;
  ReadAndParseElement(data_name, element);
  bool found(RemovePmidFromOnlineList(pmid_name->string(), element));
  if (found) {
    holders = static_cast<int64_t>(element.element_size());
    element.add_offline_pmid_name(pmid_name->string());
    SerialiseAndSaveElement(element);
  }
}

void DataElementsManager::MoveNodeToOnline(const Identity& data_name, const PmidName& pmid_name) {
  CheckDataElementExists(data_name);
  protobuf::MetadataElement element;
  ReadAndParseElement(data_name, element);
  bool found(RemovePmidFromOfflineList(pmid_name->string(), element));
  if (found) {
    element.add_online_pmid_name(pmid_name->string());
    SerialiseAndSaveElement(element);
  }
}

void DataElementsManager::AddOnlinePmid(const Identity& data_name,
                                        const PmidName& online_pmid_name) {
  CheckDataElementExists(data_name);
  protobuf::MetadataElement element;
  ReadAndParseElement(data_name, element);
  element.add_online_pmid_name(online_pmid_name->string());
  SerialiseAndSaveElement(element);
}

void DataElementsManager::RemoveOnlinePmid(const Identity& data_name,
                                           const PmidName& online_pmid_name) {
  CheckDataElementExists(data_name);
  protobuf::MetadataElement element;
  ReadAndParseElement(data_name, element);
  bool found(RemovePmidFromOnlineList(online_pmid_name->string(), element));
  if (found)
    SerialiseAndSaveElement(element);
}

void DataElementsManager::AddOfflinePmid(const Identity& data_name,
                                         const PmidName& offline_pmid_name) {
  CheckDataElementExists(data_name);
  protobuf::MetadataElement element;
  ReadAndParseElement(data_name, element);
  element.add_offline_pmid_name(offline_pmid_name->string());
  SerialiseAndSaveElement(element);
}

void DataElementsManager::RemoveOfflinePmid(const Identity& data_name,
                                            const PmidName& offline_pmid_name) {
  CheckDataElementExists(data_name);
  protobuf::MetadataElement element;
  ReadAndParseElement(data_name, element);

  // remove the pmid
  std::vector<std::string> pmid_names;
  for (int n(0); n < element.element_size(); ++n) {
    if (element.offline_pmid_name(n) != offline_pmid_name->string())
      pmid_names.push_back(element.offline_pmid_name(n));
  }
  element.clear_offline_pmid_name();
  for (auto& pmid_name : pmid_names)
    element.add_offline_pmid_name(pmid_name);

  // Save the result
  SerialiseAndSaveElement(element);
}

void DataElementsManager::CheckDataElementExists(const Identity& data_name) {
  if (!boost::filesystem::exists(vault_metadata_dir_ / EncodeToBase64(data_name))) {
    LOG(kError) << "Failed to find data ID: " << Base64Substr(data_name);
    ThrowError(NfsErrors::failed_to_find_managed_element);
  }
}

void DataElementsManager::ReadAndParseElement(const Identity& data_name,
                                              protobuf::MetadataElement& element) {
  NonEmptyString serialised_element(ReadFile(vault_metadata_dir_ / EncodeToBase64(data_name)));
  if (!element.ParseFromString(serialised_element.string())) {
    LOG(kError) << "Failed to parse data ID: " << Base64Substr(data_name);
    ThrowError(NfsErrors::managed_element_parsing_error);
  }
}

void DataElementsManager::SerialiseAndSaveElement(const protobuf::MetadataElement& element) {
  std::string serialised_element(element.SerializeAsString());
  if (serialised_element.empty()) {
    LOG(kError) << "Failed to serialise data ID: " << Base64Substr(element.data_name());
    ThrowError(NfsErrors::managed_element_serialisation_error);
  }

  if (!WriteFile(vault_metadata_dir_ / EncodeToBase64(element.data_name()), serialised_element)) {
    LOG(kError) << "Failed to write data ID: " << Base64Substr(element.data_name());
    ThrowError(NfsErrors::managed_element_serialisation_error);
  }
}


}  // namespace vault

}  // namespace maidsafe
