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

#ifndef MAIDSAFE_NFS_DATA_ELEMENTS_MANAGER_H_
#define MAIDSAFE_NFS_DATA_ELEMENTS_MANAGER_H_

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/types.h"

namespace maidsafe {

namespace nfs {

namespace protobuf { class DataElementsManaged; }

class DataElementsManager {
 public:
  explicit DataElementsManager(const boost::filesystem::path& vault_root_dir);
  void AddDataElement(const Identity& data_id,
                      int32_t element_size,
                      const Identity& online_pmid_id,
                      const Identity& offline_pmid_id);
  void RemoveDataElement(const Identity& data_id);
  void MoveNodeToOffline(const Identity& data_id, const Identity& pmid_id, int64_t& holders);
  void MoveNodeToOnline(const Identity& data_id, const Identity& pmid_id);

  void AddOnlinePmid(const Identity& data_id, const Identity& online_pmid_id);
  void RemoveOnlinePmid(const Identity& data_id, const Identity& online_pmid_id);
  void AddOfflinePmid(const Identity& data_id, const Identity& offline_pmid_id);
  void RemoveOfflinePmid(const Identity& data_id, const Identity& offline_pmid_id);

 private:
  boost::filesystem::path vault_metadata_dir_;

  void CheckDataElementExists(const Identity& data_id);
  void ReadAndParseElement(const Identity& data_id, protobuf::DataElementsManaged& element);
  void SerialiseAndSaveElement(const protobuf::DataElementsManaged & element);
};

}  // namespace nfs

}  // namespace maidsafe

#endif  // MAIDSAFE_NFS_DATA_ELEMENTS_MANAGER_H_
