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

#ifndef MAIDSAFE_VAULT_DATA_MANAGER_VALUE_H_
#define MAIDSAFE_VAULT_DATA_MANAGER_VALUE_H_

#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include "maidsafe/common/types.h"
#include "maidsafe/common/visualiser_log.h"
#include "maidsafe/common/data_types/data_name_variant.h"

#include "maidsafe/routing/routing_api.h"

#include "maidsafe/vault/data_manager/data_manager.pb.h"
#include "maidsafe/vault/types.h"

namespace maidsafe {

namespace vault {

// not thread safe
class DataManagerValue {
 public:
  DataManagerValue();
  explicit DataManagerValue(const std::string& serialised_value);
  DataManagerValue(DataManagerValue&& other) MAIDSAFE_NOEXCEPT;
  DataManagerValue(uint64_t size);
  std::string Serialise() const;

  DataManagerValue& operator=(const DataManagerValue& other);

  void AddPmid(const PmidName& pmid_name);
  void RemovePmid(const PmidName& pmid_name);
  bool HasTarget(const PmidName& pmid_name) const;
  std::vector<PmidName> AllPmids() const { return pmids_; }
  std::vector<PmidName> online_pmids(const std::vector<NodeId>& close_nodes) const;

  // Prune the oldest offline node
  bool NeedToPrune(const std::vector<NodeId>& close_nodes, PmidName& pmid_node_to_remove) const;

  std::string Print() const;
  uint64_t chunk_size() const { return size_; }
  void SetChunkSize(const uint64_t chunk_size) { size_ = chunk_size; }

  static DataManagerValue Resolve(const std::vector<DataManagerValue>& values);
  friend bool operator==(const DataManagerValue& lhs, const DataManagerValue& rhs);

// private: MAID-357
  DataManagerValue(const DataManagerValue&);

 private:
  void PrintRecords();
  uint64_t size_;
  std::vector<PmidName> pmids_;
};

bool operator==(const DataManagerValue& lhs, const DataManagerValue& rhs);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_VALUE_H_
