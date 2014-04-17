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

#include "maidsafe/common/types.h"
#include "maidsafe/common/data_types/data_name_variant.h"

#include "maidsafe/vault/data_manager/data_manager.pb.h"
#include "maidsafe/vault/types.h"

namespace maidsafe {

namespace vault {

// not thread safe
class DataManagerValue {
 public:
  explicit DataManagerValue(const std::string& serialised_metadata_value);
  DataManagerValue(DataManagerValue&& other);
  DataManagerValue(const PmidName& pmid_name, int32_t size);
  std::string Serialise() const;

  DataManagerValue& operator=(const DataManagerValue& other);

  void AddPmid(const PmidName& pmid_name);
  void RemovePmid(const PmidName& pmid_name);
  void IncrementSubscribers() {
    ++subscribers_;
    GLOG() << "DataManager increase subscribers to " << subscribers_;
  }
  int64_t DecrementSubscribers();
  void SetPmidOnline(const PmidName& pmid_name);
  void SetPmidOffline(const PmidName& pmid_name);
  int64_t Subscribers() const { return subscribers_; }
  std::set<PmidName> AllPmids() const;
  std::set<PmidName> online_pmids() const { return online_pmids_; }

  friend bool operator==(const DataManagerValue& lhs, const DataManagerValue& rhs);
#ifdef MAIDSAFE_APPLE  // BEFORE_RELEASE This copy constructor definition is to allow building
                       // on mac with clang 3.3, should be removed if clang is updated on mac.
  DataManagerValue(const DataManagerValue& other)
      : subscribers_(other.subscribers_), size_(other.size_), online_pmids_(other.online_pmids_),
        offline_pmids_(other.offline_pmids_) {}
#else

 private:
  DataManagerValue(const DataManagerValue&);
#endif

 private:
  void PrintRecords();
  int64_t subscribers_;
  int32_t size_;
  std::set<PmidName> online_pmids_, offline_pmids_;
};

bool operator==(const DataManagerValue& lhs, const DataManagerValue& rhs);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_VALUE_H_
