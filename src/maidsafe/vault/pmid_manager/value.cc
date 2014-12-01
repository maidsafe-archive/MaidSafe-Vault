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

#include "maidsafe/routing/parameters.h"

#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.pb.h"

namespace maidsafe {
namespace vault {

PmidManagerValue::PmidManagerValue()
    : stored_total_size(0), lost_total_size(0), offered_space(0) {}

PmidManagerValue::PmidManagerValue(const uint64_t& stored_total_size_in,
                                   const uint64_t& lost_total_size_in,
                                   const uint64_t& offered_space_in)
    : stored_total_size(stored_total_size_in), lost_total_size(lost_total_size_in),
      offered_space(offered_space_in) {}

PmidManagerValue::PmidManagerValue(const std::string &serialised_value)
    : stored_total_size(0), lost_total_size(0), offered_space(0) {
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
  offered_space = proto_value.offered_space();
}

PmidManagerValue::PmidManagerValue(const PmidManagerValue& other)
    : stored_total_size(other.stored_total_size),
      lost_total_size(other.lost_total_size),
      offered_space(other.offered_space) {}

PmidManagerValue::PmidManagerValue(PmidManagerValue&& other)
    : stored_total_size(std::move(other.stored_total_size)),
      lost_total_size(std::move(other.lost_total_size)),
      offered_space(std::move(other.offered_space)) {}

PmidManagerValue& PmidManagerValue::operator=(PmidManagerValue other) {
  using std::swap;
  swap(stored_total_size, other.stored_total_size);
  swap(lost_total_size, other.lost_total_size);
  swap(offered_space, other.offered_space);
  return *this;
}

void PmidManagerValue::PutData(uint64_t size) {
  stored_total_size += size;
}

void PmidManagerValue::DeleteData(uint64_t size) {
  if (stored_total_size < size) {
    LOG(kError) << "invalid stored_total_size " << stored_total_size;
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::invalid_parameter));
  }
  stored_total_size -= size;
}

void PmidManagerValue::HandleLostData(uint64_t size) {
  DeleteData(size);
  lost_total_size += size;
}

void PmidManagerValue::HandleFailure(uint64_t size) {
  HandleLostData(size);
}

void PmidManagerValue::SetAvailableSize(const int64_t& available_size) {
  offered_space = available_size;
}

void PmidManagerValue::UpdateAccount(int32_t diff_size) {
  stored_total_size =
      (static_cast<int64_t>(stored_total_size) < diff_size) ? 0 : (stored_total_size - diff_size);
  lost_total_size += diff_size;
}

// BEFORE_RELEASE check if group can be deleted with below check
detail::GroupDbMetaDataStatus PmidManagerValue::GroupStatus() {
  return (offered_space == 0 ?
      detail::GroupDbMetaDataStatus::kGroupEmpty : detail::GroupDbMetaDataStatus::kGroupNonEmpty);
}

std::string PmidManagerValue::Serialise() const {
  protobuf::PmidManagerValue proto_value;
  proto_value.set_stored_total_size(stored_total_size);
  proto_value.set_lost_total_size(lost_total_size);
  proto_value.set_offered_space(offered_space);
  return proto_value.SerializeAsString();
}

bool operator==(const PmidManagerValue& lhs, const PmidManagerValue& rhs) {
  return lhs.stored_total_size == rhs.stored_total_size &&
         lhs.lost_total_size == rhs.lost_total_size &&
         lhs.offered_space == rhs.offered_space;
}

std::string PmidManagerValue::Print() const {
  std::stringstream stream;
  stream << "\t[stored_total_size," << stored_total_size
         << "] [lost_total_size," << lost_total_size
         << "] [offered_space," << offered_space << "]";
  return stream.str();
}

PmidManagerValue PmidManagerValue::Resolve(const std::vector<PmidManagerValue>& values) {
  size_t size(values.size());
  if (size < (routing::Parameters::group_size + 1) / 2)
    BOOST_THROW_EXCEPTION(MakeError(VaultErrors::too_few_entries_to_resolve));
  std::vector<int64_t> stored_total_size, lost_total_size, offered_space;
  for (const auto& value : values) {
    stored_total_size.emplace_back(value.stored_total_size);
    lost_total_size.emplace_back(value.lost_total_size);
    offered_space.emplace_back(value.offered_space);
  }

  PmidManagerValue value;
  value.stored_total_size = Median(stored_total_size);
  value.lost_total_size = Median(lost_total_size);
  value.offered_space = Median(offered_space);

  return value;
}

}  // namespace vault

}  // namespace maidsafe
