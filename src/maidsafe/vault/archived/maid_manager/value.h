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

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_VALUE_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_VALUE_H_

#include <cstdint>
#include <string>
#include <vector>

#include "maidsafe/vault/types.h"

namespace maidsafe {

namespace vault {

namespace test {
  class MaidManagerServiceTest;
}

class MaidManagerValue {
 public:
  enum class Status {
    kOk,
    kLowSpace,
    kNoSpace
  };
  MaidManagerValue();
  MaidManagerValue(uint64_t data_stored, uint64_t space_available);
  MaidManagerValue(const MaidManagerValue& other);
  MaidManagerValue(MaidManagerValue&& other);
  MaidManagerValue& operator=(MaidManagerValue other);
  explicit MaidManagerValue(const std::string& serialised_value);

  std::string Serialise() const;
  template <typename Data>
  Status AllowPut(const Data& data) const;
  void PutData(uint64_t size);
  void DeleteData(uint64_t size);
  std::string Print() const;

  static MaidManagerValue Resolve(const std::vector<MaidManagerValue>& values);

  friend void swap(MaidManagerValue& lhs, MaidManagerValue& rhs);
  friend bool operator==(const MaidManagerValue& lhs, const MaidManagerValue& rhs);

  uint64_t data_stored;
  uint64_t space_available;
};


template <>
MaidManagerValue::Status MaidManagerValue::AllowPut(const passport::PublicPmid& data) const;
template <>
MaidManagerValue::Status MaidManagerValue::AllowPut(const passport::PublicAnpmid& data) const;
template <>
MaidManagerValue::Status MaidManagerValue::AllowPut(const passport::PublicMaid& data) const;
template <>
MaidManagerValue::Status MaidManagerValue::AllowPut(const passport::PublicAnmaid& data) const;


template <typename Data>
MaidManagerValue::Status MaidManagerValue::AllowPut(const Data& data) const {
  auto size(data.Serialise()->string().size());
  LOG(kVerbose) << "MaidManagerValue::AllowPut data " << HexSubstr(data.name().value)
    << " has size of " << size << " trying to put into account provding "
    << space_available << " total available_size by far";
  if (space_available < (static_cast<uint64_t>(data_stored + size)))
    return Status::kNoSpace;

  return ((3 * space_available / 100) < static_cast<uint64_t>(data_stored + size))
    ? Status::kLowSpace : Status::kOk;
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_VALUE_H_
