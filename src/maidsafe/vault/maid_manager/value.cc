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

#include "maidsafe/vault/maid_manager/value.h"

#include <utility>
#include <limits>

#include "maidsafe/vault/utils.h"
#include "maidsafe/routing/parameters.h"
#include "maidsafe/vault/maid_manager/maid_manager.pb.h"

namespace maidsafe {

namespace vault {

MaidManagerValue::MaidManagerValue()
    : data_stored(0), space_available(std::numeric_limits<uint64_t>().max()) {}

MaidManagerValue::MaidManagerValue(uint64_t data_stored, uint64_t space_available)
    : data_stored(data_stored), space_available(space_available) {}

MaidManagerValue::MaidManagerValue(const MaidManagerValue& other)
    : data_stored(other.data_stored), space_available(other.space_available) {}

MaidManagerValue::MaidManagerValue(MaidManagerValue&& other)
    : data_stored(std::move(other.data_stored)),
      space_available(std::move(other.space_available)) {}

MaidManagerValue& MaidManagerValue::operator=(MaidManagerValue other) {
  swap(*this, other);
  return *this;
}

MaidManagerValue::MaidManagerValue(const std::string& serialised_value) {
  protobuf::MaidManagerValue maid_manager_value_proto;
  if (!maid_manager_value_proto.ParseFromString(serialised_value)) {
    LOG(kError) << "Failed to read or parse serialised maid manager value";
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
  }
  data_stored = maid_manager_value_proto.data_stored();
  space_available = maid_manager_value_proto.space_available();
}

void MaidManagerValue::PutData(uint64_t size) {
  data_stored += size;
  space_available -= size;
}

void MaidManagerValue::DeleteData(uint64_t size) {
  data_stored -= size;
  space_available += size;
}

template <>
MaidManagerValue::Status MaidManagerValue::AllowPut(
    const passport::PublicPmid& /*data*/) const {
  return Status::kOk;
}

template <>
MaidManagerValue::Status MaidManagerValue::AllowPut(
    const passport::PublicAnpmid& /*data*/) const {
  return Status::kOk;
}

template <>
MaidManagerValue::Status MaidManagerValue::AllowPut(
    const passport::PublicMaid& /*data*/) const {
  assert(false && "Storing PublicMaid is not allowed on existing Account");
  return Status::kNoSpace;
}

template <>
MaidManagerValue::Status MaidManagerValue::AllowPut(
    const passport::PublicAnmaid& /*data*/) const {
  assert(false && "Storing PublicMaid is not allowed on existing Account");
  return Status::kNoSpace;
}

std::string MaidManagerValue::Serialise() const {
  protobuf::MaidManagerValue maid_manager_value_proto;
  maid_manager_value_proto.set_data_stored(data_stored);
  maid_manager_value_proto.set_space_available(space_available);
  return maid_manager_value_proto.SerializeAsString();
}

MaidManagerValue MaidManagerValue::Resolve(const std::vector<MaidManagerValue>& values) {
  size_t size(values.size());
  if (size < (routing::Parameters::group_size + 1) / 2)
    BOOST_THROW_EXCEPTION(MakeError(VaultErrors::too_few_entries_to_resolve));
  std::vector<int64_t> data_stored, space_available;
  for (const auto& value : values) {
    data_stored.emplace_back(value.data_stored);
    space_available.emplace_back(value.space_available);
  }

  MaidManagerValue value;
  value.data_stored = Median(data_stored);
  value.space_available = Median(space_available);

  return value;
}

void swap(MaidManagerValue& lhs, MaidManagerValue& rhs) {
  using std::swap;
  swap(lhs.data_stored, rhs.data_stored);
  swap(lhs.space_available, rhs.space_available);
}

bool operator==(const MaidManagerValue& lhs, const MaidManagerValue& rhs) {
  return lhs.data_stored == rhs.data_stored && lhs.space_available == rhs.space_available;
}

std::string MaidManagerValue::Print() const {
  std::stringstream stream;
  stream << "\tspace available," << space_available << " with " << "data stored, "
         << data_stored;
  return stream.str();
}

}  // namespace vault

}  // namespace maidsafe
