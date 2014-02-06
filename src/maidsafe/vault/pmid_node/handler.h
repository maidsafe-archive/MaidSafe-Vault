/*  Copyright 2012 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.maidsafe.net/licenses

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#ifndef MAIDSAFE_VAULT_PMID_NODE_HANDLER_H_
#define MAIDSAFE_VAULT_PMID_NODE_HANDLER_H_

#include <string>
#include <vector>

#include "maidsafe/data_store/data_store.h"
#include "maidsafe/data_store/permanent_store.h"
#include "maidsafe/data_store/data_buffer.h"
#include "maidsafe/data_types/data_name_variant.h"

namespace maidsafe {
namespace vault {

class PmidNodeHandler {
 public:
  explicit PmidNodeHandler(const boost::filesystem::path vault_root_dir);

  template <typename Data>
  Data Get(const typename Data::Name& data_name);

  template <typename Data>
  void Put(const Data& data);

  template <typename DataName>
  void Delete(const DataName& data_name);

  boost::filesystem::path GetDiskPath() const;
  std::vector<DataNameVariant> GetAllDataNames() const;
  DiskUsage AvailableSpace() const;

 private:
  boost::filesystem::space_info space_info_;
  DiskUsage disk_total_;
  DiskUsage permanent_size_;
  data_store::PermanentStore permanent_data_store_;
};

template <typename Data>
Data PmidNodeHandler::Get(const typename Data::Name& data_name) {
  DataNameVariant data_name_variant(data_name);
  Data data(data_name,
            typename Data::serialised_type(permanent_data_store_.Get(data_name_variant)));
  return data;
}


template <typename Data>
void PmidNodeHandler::Put(const Data& data) {
  GLOG() << "PmidNode storing chunk " << HexSubstr(data.name().value.string())
         << " with content " << HexSubstr(data.Serialise().data);
  permanent_data_store_.Put(DataNameVariant(data.name()), data.Serialise().data);
}

template <typename DataName>
void PmidNodeHandler::Delete(const DataName& data_name) {
  permanent_data_store_.Delete(DataNameVariant(data_name));
}

}  // namespace vault
}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_NODE_HANDLER_H_
