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

#ifndef MAIDSAFE_VAULT_METADATA_MANAGER_DATA_ELEMENTS_MANAGER_H_
#define MAIDSAFE_VAULT_METADATA_MANAGER_DATA_ELEMENTS_MANAGER_H_

#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/types.h"

#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

namespace protobuf { class MetadataElement; }

class DataElementsManager {
 public:
  explicit DataElementsManager(const boost::filesystem::path& vault_root_dir);
  void AddDataElement(const Identity& data_name,
                      int32_t element_size,
                      const PmidName& online_pmid_name,
                      const PmidName& offline_pmid_name);
  void RemoveDataElement(const Identity& data_name);
  int64_t DecreaseDataElement(const Identity& data_name);
  void MoveNodeToOffline(const Identity& data_name, const PmidName& pmid_name, int64_t& holders);
  void MoveNodeToOnline(const Identity& data_name, const PmidName& pmid_name);

  void AddOnlinePmid(const Identity& data_name, const PmidName& online_pmid_name);
  void RemoveOnlinePmid(const Identity& data_name, const PmidName& online_pmid_name);
  void AddOfflinePmid(const Identity& data_name, const PmidName& offline_pmid_name);
  void RemoveOfflinePmid(const Identity& data_name, const PmidName& offline_pmid_name);

  std::vector<Identity> GetOnlinePmid(const Identity& data_id);

 private:
  boost::filesystem::path vault_metadata_dir_;

  void CheckDataElementExists(const Identity& data_name);
  void ReadAndParseElement(const Identity& data_name, protobuf::MetadataElement& element);
  void SerialiseAndSaveElement(const protobuf::MetadataElement& element);
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_METADATA_MANAGER_DATA_ELEMENTS_MANAGER_H_
