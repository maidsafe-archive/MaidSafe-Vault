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

#include "maidsafe/vault/metadata_manager/metadata_handler.h"

#include <string>

#include "maidsafe/vault/utils.h"



namespace fs = boost::filesystem;

namespace maidsafe {

namespace vault {

namespace detail {

fs::path GetPath(const std::string& data_name,
                 int32_t data_type_enum_value,
                 const fs::path& root) {
  return root / (EncodeToBase32(data_name) + std::to_string(data_type_enum_value));
}

}  // namespace detail

MetadataHandler::MetadataHandler(const fs::path& vault_root_dir, const NodeId &this_node_id)
    : kMetadataRoot_([vault_root_dir]()->boost::filesystem::path {
                       auto path(vault_root_dir / "metadata");
                       detail::InitialiseDirectory(path);
                       return path;
                     } ()),
      metadata_db_(new MetadataDb(kMetadataRoot_)),
      kThisNodeId_(this_node_id),
      mutex_(),
      sync_(metadata_db_.get(), kThisNodeId_) {
  detail::InitialiseDirectory(kMetadataRoot_);
}

void MetadataHandler::DeleteRecord(const DataNameVariant& /*record_name*/) {

}

void MetadataHandler::AddLocalUnresolvedEntry(const MetadataUnresolvedEntry& unresolved_entry) {
  std::lock_guard<std::mutex> lock(mutex_);
  sync_.AddLocalEntry(unresolved_entry);
}

std::vector<DataNameVariant> MetadataHandler::GetRecordNames() const {
  return metadata_db_->GetKeys();
}

void MetadataHandler::ReplaceNodeInSyncList(const DataNameVariant& /*record_name*/,  //FIXME in Sync
                                            const NodeId& old_node, const NodeId& new_node) {
  // FIXME(Prakash) Need to pass record_name to sync
  sync_.ReplaceNode(old_node, new_node);
}

//void MetadataHandler::PutMetadata(const protobuf::Metadata& /*proto_metadata*/) {
//  if (!proto_metadata.IsInitialized() ||
//      !Identity(proto_metadata.name()).IsInitialised() ||
//      proto_metadata.size() < 1 ||
//      proto_metadata.subscribers() < 1) {
//    LOG(kError) << "Copied an invalid metadata file";
//    ThrowError(CommonErrors::invalid_parameter);
//  }
//  auto path(detail::GetPath(proto_metadata.name(), proto_metadata.type(), kMetadataRoot_));
//  std::string serialised_content(proto_metadata.SerializeAsString());
//  assert(!serialised_content.empty());
//  if (!WriteFile(path, serialised_content)) {
//    LOG(kError) << "Failed to write metadata file " << path;
//    ThrowError(CommonErrors::filesystem_io_error);
//  }
//}


//  namespace {

//  bool RemovePmidFromOnlineList(const std::string& pmid, protobuf::Metadata& element) {
//    std::vector<std::string> pmid_names;
//    bool found(false);
//    for (int n(0); n < element.online_pmid_name_size(); ++n) {
//      if (element.online_pmid_name(n) == pmid)
//        found = true;
//      else
//        pmid_names.push_back(element.online_pmid_name(n));
//    }

//    if (found) {
//      element.clear_online_pmid_name();
//      for (auto& pmid_name : pmid_names)
//        element.add_online_pmid_name(pmid_name);
//    }

//    return found;
//  }

//  bool RemovePmidFromOfflineList(const std::string& pmid, protobuf::Metadata& element) {
//    std::vector<std::string> pmid_names;
//    bool found(false);
//    for (int n(0); n < element.offline_pmid_name_size(); ++n) {
//      if (element.offline_pmid_name(n) == pmid)
//        found = true;
//      else
//        pmid_names.push_back(element.offline_pmid_name(n));
//    }

//    if (found) {
//      element.clear_offline_pmid_name();
//      for (auto& pmid_name : pmid_names)
//        element.add_offline_pmid_name(pmid_name);
//    }

//    return found;
//  }

//  }  // namespace

//  const boost::filesystem::path kVaultDirectory("meta_data_manager");

//  void MetadataHandler::AddDataElement(const Identity& data_name,
//                                           int32_t element_size,
//                                           const PmidName& online_pmid_name,
//                                           const PmidName& offline_pmid_name) {
//    protobuf::Metadata element;
//    try {
//      CheckDataElementExists(data_name);
//      // Increase counter
//      ReadAndParseElement(data_name, element);
//      int64_t number_stored(element.number_stored());
//      element.set_number_stored(number_stored + 1);
//    }
//    catch(...) {
//      // Add new entry
//      element.set_data_name(data_name.string());
//      element.set_element_size(element_size);
//      element.set_number_stored(1);
//      element.add_online_pmid_name(online_pmid_name->string());
//      element.add_offline_pmid_name(offline_pmid_name->string());
//    }

//    SerialiseAndSaveElement(element);
//  }

//  void MetadataHandler::RemoveDataElement(const Identity& data_name) {
//    CheckDataElementExists(data_name);
//    boost::filesystem::remove(vault_metadata_dir_ / EncodeToBase64(data_name));
//  }

//  int64_t MetadataHandler::DecreaseDataElement(const Identity& data_name) {
//    int64_t number_stored(-1);
//    try {
//      protobuf::Metadata element;
//      CheckDataElementExists(data_name);
//      // Decrease counter
//      ReadAndParseElement(data_name, element);
//      number_stored = element.number_stored();
//      // prevent over decreasing, return 0 to trigger a removal in that case
//      if (number_stored > 0) {
//        --number_stored;
//        element.set_number_stored(number_stored);
//        SerialiseAndSaveElement(element);
//      } else {
//        number_stored = 0;
//      }
//    }
//    catch(...) {
//      LOG(kError) << "Failed to find element of " << HexSubstr(data_name.string());
//    }
//    return number_stored;
//  }

//  void MetadataHandler::MoveNodeToOffline(const Identity& data_name,
//                                              const PmidName& pmid_name,
//                                              int64_t& holders) {
//    CheckDataElementExists(data_name);
//    protobuf::Metadata element;
//    ReadAndParseElement(data_name, element);
//    bool found(RemovePmidFromOnlineList(pmid_name->string(), element));
//    if (found) {
//      holders = static_cast<int64_t>(element.online_pmid_name_size());
//      element.add_offline_pmid_name(pmid_name->string());
//      SerialiseAndSaveElement(element);
//    }
//  }

//  void MetadataHandler::MoveNodeToOnline(const Identity& data_name, const PmidName& pmid_name) {
//    CheckDataElementExists(data_name);
//    protobuf::Metadata element;
//    ReadAndParseElement(data_name, element);
//    bool found(RemovePmidFromOfflineList(pmid_name->string(), element));
//    if (found) {
//      element.add_online_pmid_name(pmid_name->string());
//      SerialiseAndSaveElement(element);
//    }
//  }

//  void MetadataHandler::AddOnlinePmid(const Identity& data_name,
//                                          const PmidName& online_pmid_name) {
//    CheckDataElementExists(data_name);
//    protobuf::Metadata element;
//    ReadAndParseElement(data_name, element);
//    element.add_online_pmid_name(online_pmid_name->string());
//    SerialiseAndSaveElement(element);
//  }

//  void MetadataHandler::RemoveOnlinePmid(const Identity& data_name,
//                                             const PmidName& online_pmid_name) {
//    CheckDataElementExists(data_name);
//    protobuf::Metadata element;
//    ReadAndParseElement(data_name, element);
//    bool found(RemovePmidFromOnlineList(online_pmid_name->string(), element));
//    if (found)
//      SerialiseAndSaveElement(element);
//  }

//  void MetadataHandler::AddOfflinePmid(const Identity& data_name,
//                                           const PmidName& offline_pmid_name) {
//    CheckDataElementExists(data_name);
//    protobuf::Metadata element;
//    ReadAndParseElement(data_name, element);
//    element.add_offline_pmid_name(offline_pmid_name->string());
//    SerialiseAndSaveElement(element);
//  }

//  void MetadataHandler::RemoveOfflinePmid(const Identity& data_name,
//                                              const PmidName& offline_pmid_name) {
//    CheckDataElementExists(data_name);
//    protobuf::Metadata element;
//    ReadAndParseElement(data_name, element);
//    bool found(RemovePmidFromOfflineList(offline_pmid_name->string(), element));
//    if (found)
//      SerialiseAndSaveElement(element);
//  }

//  std::vector<Identity> MetadataHandler::GetOnlinePmid(const Identity& data_id) {
//    CheckDataElementExists(data_id);
//    protobuf::Metadata element;
//    ReadAndParseElement(data_id, element);
//    std::vector<Identity> online_pmids;
//    for (int n(0); n < element.online_pmid_name_size(); ++n) {
//      online_pmids.push_back(Identity(element.online_pmid_name(n)));
//    }
//    return online_pmids;
//  }

//  void MetadataHandler::CheckDataElementExists(const Identity& data_name) {
//    if (!boost::filesystem::exists(vault_metadata_dir_ / EncodeToBase64(data_name))) {
//      LOG(kError) << "Failed to find data ID: " << Base64Substr(data_name);
//      ThrowError(CommonErrors::no_such_element);
//    }
//  }

}  // namespace vault

}  // namespace maidsafe
