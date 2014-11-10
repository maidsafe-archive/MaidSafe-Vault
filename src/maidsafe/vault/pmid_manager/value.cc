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

#include "maidsafe/vault/pmid_manager/value.h"

#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/vault/pmid_manager/pmid_manager.pb.h"

namespace maidsafe {
namespace vault {

PmidManagerValue::PmidManagerValue()
    : stored_total_size(0), lost_total_size(0), claimed_available_size(0) {}

PmidManagerValue::PmidManagerValue(const std::string &serialised_value)
    : stored_total_size(0), lost_total_size(0), claimed_available_size(0) {
  LOG(kVerbose) << "PmidManagerValue parsing from " << HexSubstr(serialised_value);
  protobuf::PmidManagerValue proto_value;
  if (!proto_value.ParseFromString(serialised_value)) {
    LOG(kError) << "Failed to parse pmid value.";
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
  }
  if (!proto_value.IsInitialized()) {
    LOG(kError) << "Failed to construct pmid value.";
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::invalid_parameter));
  }
  stored_total_size = proto_value.stored_total_size();
  lost_total_size = proto_value.lost_total_size();
  claimed_available_size = proto_value.claimed_available_size();
}

PmidManagerValue::PmidManagerValue(const PmidManagerValue& other)
    : stored_total_size(other.stored_total_size),
      lost_total_size(other.lost_total_size),
      claimed_available_size(other.claimed_available_size) {}

PmidManagerValue::PmidManagerValue(PmidManagerValue&& other)
    : stored_total_size(std::move(other.stored_total_size)),
      lost_total_size(std::move(other.lost_total_size)),
      claimed_available_size(std::move(other.claimed_available_size)) {}

PmidManagerValue& PmidManagerValue::operator=(PmidManagerValue other) {
  using std::swap;
  swap(stored_total_size, other.stored_total_size);
  swap(lost_total_size, other.lost_total_size);
  swap(claimed_available_size, other.claimed_available_size);
  return *this;
}

void PmidManagerValue::PutData(int32_t size) {
  stored_total_size += size;
}

void PmidManagerValue::DeleteData(int32_t size) {
  if (stored_total_size < size) {
    LOG(kError) << "invalid stored_total_size " << stored_total_size;
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::invalid_parameter));
  }
  stored_total_size -= size;
}

void PmidManagerValue::HandleLostData(int32_t size) {
  DeleteData(size);
  lost_total_size += size;
}

void PmidManagerValue::HandleFailure(int32_t size) {
  HandleLostData(size);
  claimed_available_size = 0;
}

void PmidManagerValue::SetAvailableSize(const int64_t& available_size) {
  claimed_available_size = available_size;
}


// BEFORE_RELEASE check if group can be deleted with below check
detail::GroupDbMetaDataStatus PmidManagerValue::GroupStatus() {
  return (claimed_available_size == 0 ?
      detail::GroupDbMetaDataStatus::kGroupEmpty : detail::GroupDbMetaDataStatus::kGroupNonEmpty);
}

std::string PmidManagerValue::Serialise() const {
  protobuf::PmidManagerValue proto_value;
  proto_value.set_stored_total_size(stored_total_size);
  proto_value.set_lost_total_size(lost_total_size);
  proto_value.set_claimed_available_size(claimed_available_size);
  return proto_value.SerializeAsString();
}

bool operator==(const PmidManagerValue& lhs, const PmidManagerValue& rhs) {
  return lhs.stored_total_size == rhs.stored_total_size &&
         lhs.lost_total_size == rhs.lost_total_size &&
         lhs.claimed_available_size == rhs.claimed_available_size;
}

std::string PmidManagerValue::Print() const {
  std::stringstream stream;
  stream << "\t[stored_total_size," << stored_total_size
         << "] [lost_total_size," << lost_total_size
         << "] [claimed_available_size," << claimed_available_size << "]";
  return stream.str();
}

}  // namespace vault
}  // namespace maidsafe
