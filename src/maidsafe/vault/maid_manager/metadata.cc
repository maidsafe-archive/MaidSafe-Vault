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

#include "maidsafe/vault/maid_manager/metadata.h"

#include <utility>
#include <limits>

#include "maidsafe/vault/maid_manager/maid_manager.pb.h"
#include "maidsafe/vault/pmid_manager/metadata.h"

namespace maidsafe {

namespace vault {

MaidManagerMetadata::MaidManagerMetadata()
    : data_stored_(0), space_available_(std::numeric_limits<int64_t>().max()) {}

MaidManagerMetadata::MaidManagerMetadata(int64_t data_stored, int64_t space_available)
    : data_stored_(data_stored), space_available_(space_available) {}

MaidManagerMetadata::MaidManagerMetadata(const MaidManagerMetadata& other)
    : data_stored_(other.data_stored_), space_available_(other.space_available_) {}

MaidManagerMetadata::MaidManagerMetadata(MaidManagerMetadata&& other)
    : data_stored_(std::move(other.data_stored_)),
      space_available_(std::move(other.space_available_)) {}

MaidManagerMetadata& MaidManagerMetadata::operator=(MaidManagerMetadata other) {
  swap(*this, other);
  return *this;
}

MaidManagerMetadata::MaidManagerMetadata(const std::string& serialised_metadata_value) {
  protobuf::MaidManagerMetadata maid_manager_metadata_proto;
  if (!maid_manager_metadata_proto.ParseFromString(serialised_metadata_value)) {
    LOG(kError) << "Failed to read or parse serialised maid manager value";
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
  }
  data_stored_ = maid_manager_metadata_proto.data_stored();
  space_available_ = maid_manager_metadata_proto.space_available();
  if (data_stored_ < 0) {
    LOG(kError) << "negative data stored " << data_stored_;
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::invalid_parameter));
  }
}

void MaidManagerMetadata::PutData(int64_t size) {
  data_stored_ += size;
  space_available_ -= size;
}

void MaidManagerMetadata::DeleteData(int64_t size) {
  data_stored_ -= size;
  if (data_stored_ < 0) {
    LOG(kError) << "negative data stored " << data_stored_;
    data_stored_ += size;
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::invalid_parameter));
  }
  space_available_ += size;
}

template <>
MaidManagerMetadata::Status MaidManagerMetadata::AllowPut(
    const passport::PublicPmid& /*data*/) const {
  return Status::kOk;
}

template <>
MaidManagerMetadata::Status MaidManagerMetadata::AllowPut(
    const passport::PublicMaid& /*data*/) const {
  assert(false && "Storing PublicMaid is not allowed on existing Account");
  return Status::kNoSpace;
}

template <>
MaidManagerMetadata::Status MaidManagerMetadata::AllowPut(
    const passport::PublicAnmaid& /*data*/) const {
  assert(false && "Storing PublicMaid is not allowed on existing Account");
  return Status::kNoSpace;
}

void MaidManagerMetadata::RegisterPmid(const nfs_vault::PmidRegistration& /*pmid_registration*/) {}

void MaidManagerMetadata::UnregisterPmid(const PmidName& /*pmid_name*/) {}

void MaidManagerMetadata::UpdatePmidTotals(const PmidManagerMetadata& /*pmid_metadata*/) {}

std::string MaidManagerMetadata::Serialise() const {
  protobuf::MaidManagerMetadata maid_manager_metadata_proto;
  maid_manager_metadata_proto.set_data_stored(data_stored_);
  maid_manager_metadata_proto.set_space_available(space_available_);
  return maid_manager_metadata_proto.SerializeAsString();
}

void swap(MaidManagerMetadata& lhs, MaidManagerMetadata& rhs) {
  using std::swap;
  swap(lhs.data_stored_, rhs.data_stored_);
  swap(lhs.space_available_, rhs.space_available_);
}

bool operator==(const MaidManagerMetadata& lhs, const MaidManagerMetadata& rhs) {
  return lhs.data_stored_ == rhs.data_stored_ && lhs.space_available_ == rhs.space_available_;
}

std::string MaidManagerMetadata::Print() const {
  std::stringstream stream;
  stream << "\tspace available," << space_available_ << " with " << "data stored, "
         << data_stored_;
  return stream.str();
}

}  // namespace vault

}  // namespace maidsafe
