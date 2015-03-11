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

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_ACCOUNT_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_ACCOUNT_H_

#include <cstdint>
#include <string>
#include <vector>

#include "maidsafe/passport/types.h"

namespace maidsafe {

namespace vault {

class MaidManagerAccount {
 public:
  using AccountName = passport::PublicMaid::Name;

  enum class Status { kOk, kLowSpace, kNoSpace };
  static const uint32_t kWeight = 4;

  MaidManagerAccount() = delete;
  MaidManagerAccount(const AccountName& name, uint64_t data_stored, int64_t space_available);
  MaidManagerAccount(const MaidManagerAccount& other) = default;
  MaidManagerAccount(MaidManagerAccount&& other);
  MaidManagerAccount& operator=(const MaidManagerAccount& other) = default;
  explicit MaidManagerAccount(const std::string& serialised_account);

  template <typename Data>
  Status AllowPut(const Data& data) const;
  void PutData(uint64_t size);
  void DeleteData(uint64_t size);

  std::string serialise() const;

  AccountName name() const;
  uint64_t data_stored() const;
  uint64_t space_available() const;

  bool operator==(const MaidManagerAccount& other) const;
  bool operator!=(const MaidManagerAccount& other) const;
  bool operator<(const MaidManagerAccount& other) const;
  bool operator>(const MaidManagerAccount& other) const;
  bool operator<=(const MaidManagerAccount& other) const;
  bool operator>=(const MaidManagerAccount& other) const;

 private:
  AccountName name_;
  uint64_t data_stored_;
  uint64_t space_available_;
};

template <>
MaidManagerAccount::Status MaidManagerAccount::AllowPut(const passport::PublicPmid& data) const;
template <>
MaidManagerAccount::Status MaidManagerAccount::AllowPut(const passport::PublicAnpmid& data) const;
template <>
MaidManagerAccount::Status MaidManagerAccount::AllowPut(const passport::PublicMaid& data) const;
template <>
MaidManagerAccount::Status MaidManagerAccount::AllowPut(const passport::PublicAnmaid& data) const;

template <typename Data>
MaidManagerAccount::Status MaidManagerAccount::AllowPut(const Data& data) const {
  auto size(data.Serialise()->string().size());
  if (space_available_ < (kWeight * size))
    return Status::kNoSpace;
  return (((space_available_ + data_stored_) / 100) * 90) < (data_stored_ + kWeight * size)
      ? Status::kLowSpace : Status::kOk;
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_ACCOUNT_H_
