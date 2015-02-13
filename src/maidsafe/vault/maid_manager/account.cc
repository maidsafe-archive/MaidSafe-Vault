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

#include "maidsafe/vault/maid_manager/account.h"

#include <utility>
#include <limits>

#include "maidsafe/common/serialisation/serialisation.h"

namespace maidsafe {

namespace vault {

MaidManagerAccount::MaidManagerAccount(const AccountName& name, uint64_t data_stored,
                                       int64_t space_available)
    : name_(name), data_stored_(data_stored), space_available_(space_available) {}

MaidManagerAccount::MaidManagerAccount(MaidManagerAccount&& other)
    : name_(std::move(other.name_)), data_stored_(std::move(other.data_stored_)),
      space_available_(std::move(other.space_available_)) {}

MaidManagerAccount::MaidManagerAccount(const std::string& serialised_account)
    : name_(), data_stored_(0), space_available_(0) {
  maidsafe::ConvertFromString(serialised_account, name_, data_stored_, space_available_);
}

template <>
MaidManagerAccount::Status MaidManagerAccount::AllowPut(
    const passport::PublicPmid& /*data*/) const {
  return Status::kOk;
}

template <>
MaidManagerAccount::Status MaidManagerAccount::AllowPut(
    const passport::PublicAnpmid& /*data*/) const {
  return Status::kOk;
}

template <>
MaidManagerAccount::Status MaidManagerAccount::AllowPut(
    const passport::PublicMaid& /*data*/) const {
  assert(false && "Storing PublicMaid is not allowed on existing Account");
  return Status::kNoSpace;
}

template <>
MaidManagerAccount::Status MaidManagerAccount::AllowPut(
    const passport::PublicAnmaid& /*data*/) const {
  assert(false && "Storing PublicMaid is not allowed on existing Account");
  return Status::kNoSpace;
}

void MaidManagerAccount::PutData(uint64_t size) {
  data_stored_ += size;
  space_available_ -= size;
}

void MaidManagerAccount::DeleteData(uint64_t size) {
  data_stored_ -= size;
  space_available_ += size;
}

std::string MaidManagerAccount::serialise() const {
  return maidsafe::ConvertToString(name_, data_stored_, space_available_);
}

MaidManagerAccount::AccountName MaidManagerAccount::name() const {
  return name_;
}

uint64_t MaidManagerAccount::data_stored() const {
  return data_stored_;
}

uint64_t MaidManagerAccount::space_available() const {
  return space_available_;
}

bool MaidManagerAccount::operator==(const MaidManagerAccount& other) const {
  return std::tie(name_, data_stored_, space_available_) ==
         std::tie(other.name_, other.data_stored_, other.space_available_);
}

bool MaidManagerAccount::operator!=(const MaidManagerAccount& other) const {
  return !(*this == other);
}

bool MaidManagerAccount::operator<(const MaidManagerAccount& other) const {
  return std::tie(name_, data_stored_, space_available_) <
         std::tie(other.name_, other.data_stored_, other.space_available_);
}

bool MaidManagerAccount::operator>(const MaidManagerAccount& other) const {
  return (other < *this);
}

bool MaidManagerAccount::operator<=(const MaidManagerAccount& other) const {
  return !(other < *this);
}

bool MaidManagerAccount::operator>=(const MaidManagerAccount& other) const {
  return !(*this < other);
}

}  // namespace vault

}  // namespace maidsafe
