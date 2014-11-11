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

#include "maidsafe/routing/parameters.h"
#include "maidsafe/vault/account_transfer_handler.h"
#include "maidsafe/vault/maid_manager/maid_manager.pb.h"

namespace maidsafe {

namespace vault {

MaidManagerValue::MaidManagerValue()
    : data_stored_(0), space_available_(std::numeric_limits<int64_t>().max()) {}

MaidManagerValue::MaidManagerValue(int64_t data_stored, int64_t space_available)
    : data_stored_(data_stored), space_available_(space_available) {}

MaidManagerValue::MaidManagerValue(const MaidManagerValue& other)
    : data_stored_(other.data_stored_), space_available_(other.space_available_) {}

MaidManagerValue::MaidManagerValue(MaidManagerValue&& other)
    : data_stored_(std::move(other.data_stored_)),
      space_available_(std::move(other.space_available_)) {}

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
  data_stored_ = maid_manager_value_proto.data_stored();
  space_available_ = maid_manager_value_proto.space_available();
  if (data_stored_ < 0) {
    LOG(kError) << "negative data stored " << data_stored_;
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::invalid_parameter));
  }
}

void MaidManagerValue::PutData(int64_t size) {
  data_stored_ += size;
  space_available_ -= size;
}

void MaidManagerValue::DeleteData(int64_t size) {
  data_stored_ -= size;
  if (data_stored_ < 0) {
    LOG(kError) << "negative data stored " << data_stored_;
    data_stored_ += size;
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::invalid_parameter));
  }
  space_available_ += size;
}

template <>
MaidManagerValue::Status MaidManagerValue::AllowPut(
    const passport::PublicPmid& /*data*/) const {
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
  maid_manager_value_proto.set_data_stored(data_stored_);
  maid_manager_value_proto.set_space_available(space_available_);
  return maid_manager_value_proto.SerializeAsString();
}

MaidManagerValue MaidManagerValue::Resolve(const std::vector<MaidManagerValue>& values) {
  size_t size(values.size());
  if (size < routing::Parameters::group_size + 1 / 2)
    BOOST_THROW_EXCEPTION(MakeError(VaultErrors::too_few_entries_to_resolve));
  std::vector<int64_t> data_stored, space_available;
  for (const auto& value : values) {
    data_stored.emplace_back(value.data_stored_);
    space_available.emplace_back(value.space_available_);
  }

  auto data_stored_it(data_stored.begin() + size / 2);
  std::nth_element(data_stored.begin(), data_stored_it, data_stored.end());
  auto space_available_it(space_available.begin() + size / 2);
  std::nth_element(space_available.begin(), space_available_it, space_available.end());

  MaidManagerValue value;

  if (size % 2 == 0) {
    int64_t data_stored_first(*std::max_element(data_stored.begin(), data_stored_it)),
      data_stored_second(*data_stored_it),
      space_available_first(*std::max_element(space_available.begin(), space_available_it)),
      space_available_second(*space_available_it);

    value.data_stored_ = (data_stored_first + data_stored_second) / 2;
    value.space_available_ = (space_available_first + space_available_second) / 2;
    return value;
  } else {
    value.data_stored_ = *data_stored_it;
    value.space_available_ = *space_available_it;
    return value;
  }
}

void swap(MaidManagerValue& lhs, MaidManagerValue& rhs) {
  using std::swap;
  swap(lhs.data_stored_, rhs.data_stored_);
  swap(lhs.space_available_, rhs.space_available_);
}

bool operator==(const MaidManagerValue& lhs, const MaidManagerValue& rhs) {
  return lhs.data_stored_ == rhs.data_stored_ && lhs.space_available_ == rhs.space_available_;
}

std::string MaidManagerValue::Print() const {
  std::stringstream stream;
  stream << "\tspace available," << space_available_ << " with " << "data stored, "
         << data_stored_;
  return stream.str();
}

}  // namespace vault

}  // namespace maidsafe
