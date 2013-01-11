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

#include "maidsafe/nfs/data_elements_manager.h"

#include "boost/filesystem/operations.hpp"
#include "maidsafe/common/utils.h"
#include "maidsafe/nfs/containers_pb.h"

namespace maidsafe {

namespace nfs {

namespace {

bool RemovePmidFromOnlineList(const std::string& pmid, protobuf::DataElementsManaged& element) {
  std::vector<std::string> pmid_ids;
  bool found(false);
  for (int n(0); n < element.online_pmid_id_size(); ++n) {
    if (element.online_pmid_id(n) == pmid)
      found = true;
    else
      pmid_ids.push_back(element.online_pmid_id(n));
  }

  if (found) {
    element.clear_online_pmid_id();
    for (auto& pmid : pmid_ids)
      element.add_online_pmid_id(pmid);
  }

  return found;
}

bool RemovePmidFromOfflineList(const std::string& pmid, protobuf::DataElementsManaged& element) {
  std::vector<std::string> pmid_ids;
  bool found(false);
  for (int n(0); n < element.offline_pmid_id_size(); ++n) {
    if (element.offline_pmid_id(n) == pmid)
      found = true;
    else
      pmid_ids.push_back(element.offline_pmid_id(n));
  }

  if (found) {
    element.clear_offline_pmid_id();
    for (auto& pmid : pmid_ids)
      element.add_offline_pmid_id(pmid);
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

void DataElementsManager::AddDataElement(const Identity& data_id,
                                         int32_t element_size,
                                         const Identity& online_pmid_id,
                                         const Identity& offline_pmid_id) {
  protobuf::DataElementsManaged element;
  try {
    CheckDataElementExists(data_id);
    // Increase counter
    ReadAndParseElement(data_id, element);
    int64_t number_stored(element.number_stored());
    element.set_number_stored(number_stored + 1);
  }
  catch(...) {
    // Add new entry
    element.set_data_id(data_id.string());
    element.set_element_size(element_size);
    element.set_number_stored(1);
    element.add_online_pmid_id(online_pmid_id.string());
    element.add_offline_pmid_id(offline_pmid_id.string());
  }

  SerialiseAndSaveElement(element);
}

void DataElementsManager::RemoveDataElement(const Identity& data_id) {
  CheckDataElementExists(data_id);
  boost::filesystem::remove(vault_metadata_dir_ / EncodeToBase64(data_id));
}

void DataElementsManager::MoveNodeToOffline(const Identity& data_id,
                                            const Identity& pmid_id,
                                            int64_t& holders) {
  CheckDataElementExists(data_id);
  protobuf::DataElementsManaged element;
  ReadAndParseElement(data_id, element);
  bool found(RemovePmidFromOnlineList(pmid_id.string(), element));
  if (found) {
    holders = static_cast<int64_t>(element.online_pmid_id_size());
    element.add_offline_pmid_id(pmid_id.string());
    SerialiseAndSaveElement(element);
  }
}

void DataElementsManager::MoveNodeToOnline(const Identity& data_id, const Identity& pmid_id) {
  CheckDataElementExists(data_id);
  protobuf::DataElementsManaged element;
  ReadAndParseElement(data_id, element);
  bool found(RemovePmidFromOfflineList(pmid_id.string(), element));
  if (found) {
    element.add_online_pmid_id(pmid_id.string());
    SerialiseAndSaveElement(element);
  }
}

void DataElementsManager::AddOnlinePmid(const Identity& data_id, const Identity& online_pmid_id) {
  CheckDataElementExists(data_id);
  protobuf::DataElementsManaged element;
  ReadAndParseElement(data_id, element);
  element.add_online_pmid_id(online_pmid_id.string());
  SerialiseAndSaveElement(element);
}

void DataElementsManager::RemoveOnlinePmid(const Identity& data_id,
                                           const Identity& online_pmid_id) {
  CheckDataElementExists(data_id);
  protobuf::DataElementsManaged element;
  ReadAndParseElement(data_id, element);
  bool found(RemovePmidFromOnlineList(online_pmid_id.string(), element));
  if (found)
    SerialiseAndSaveElement(element);
}

void DataElementsManager::AddOfflinePmid(const Identity& data_id, const Identity& offline_pmid_id) {
  CheckDataElementExists(data_id);
  protobuf::DataElementsManaged element;
  ReadAndParseElement(data_id, element);
  element.add_offline_pmid_id(offline_pmid_id.string());
  SerialiseAndSaveElement(element);
}

void DataElementsManager::RemoveOfflinePmid(const Identity& data_id,
                                            const Identity& offline_pmid_id) {
  CheckDataElementExists(data_id);
  protobuf::DataElementsManaged element;
  ReadAndParseElement(data_id, element);

  // remove the pmid
  std::vector<std::string> pmid_ids;
  for (int n(0); n < element.offline_pmid_id_size(); ++n) {
    if (element.offline_pmid_id(n) != offline_pmid_id.string())
      pmid_ids.push_back(element.offline_pmid_id(n));
  }
  element.clear_offline_pmid_id();
  for (auto& pmid : pmid_ids)
    element.add_offline_pmid_id(pmid);

  // Save the result
  SerialiseAndSaveElement(element);
}

void DataElementsManager::CheckDataElementExists(const Identity& data_id) {
  if (!boost::filesystem::exists(vault_metadata_dir_ / EncodeToBase64(data_id))) {
    LOG(kError) << "Failed to find data ID: " << Base64Substr(data_id);
    ThrowError(NfsErrors::failed_to_find_managed_element);
  }
}

void DataElementsManager::ReadAndParseElement(const Identity& data_id,
                                              protobuf::DataElementsManaged& element) {
  NonEmptyString serialised_element(ReadFile(vault_metadata_dir_ / EncodeToBase64(data_id)));
  if (!element.ParseFromString(serialised_element.string())) {
    LOG(kError) << "Failed to parse data ID: " << Base64Substr(data_id);
    ThrowError(NfsErrors::managed_element_parsing_error);
  }
}

void DataElementsManager::SerialiseAndSaveElement(const protobuf::DataElementsManaged& element) {
  std::string serialised_element(element.SerializeAsString());
  if (serialised_element.empty()) {
    LOG(kError) << "Failed to serialise data ID: " << Base64Substr(element.data_id());
    ThrowError(NfsErrors::managed_element_serialisation_error);
  }

  WriteFile(vault_metadata_dir_ / EncodeToBase64(element.data_id()), serialised_element);
}


}  // namespace nfs

}  // namespace maidsafe
