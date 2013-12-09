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

#include "maidsafe/vault/pmid_manager/metadata.h"

#include "maidsafe/common/log.h"

#include "maidsafe/vault/pmid_manager/pmid_manager.pb.h"

namespace maidsafe {
namespace vault {

PmidManagerMetadata::PmidManagerMetadata()
    : pmid_name(), stored_count(0), stored_total_size(0), lost_count(0), lost_total_size(0),
      claimed_available_size(0) {}

PmidManagerMetadata::PmidManagerMetadata(const PmidName& pmid_name_in)
    : pmid_name(pmid_name_in), stored_count(0), stored_total_size(0), lost_count(0),
      lost_total_size(0), claimed_available_size(0) {}

PmidManagerMetadata::PmidManagerMetadata(const std::string &serialised_metadata)
    : pmid_name(), stored_count(0), stored_total_size(0), lost_count(0), lost_total_size(0),
      claimed_available_size(0) {
  LOG(kVerbose) << "PmidManagerMetadata parsing from " << HexSubstr(serialised_metadata);
  protobuf::PmidManagerMetadata proto_metadata;
  if (!proto_metadata.ParseFromString(serialised_metadata)) {
    LOG(kError) << "Failed to parse pmid metadata.";
    ThrowError(CommonErrors::parsing_error);
  }
  if (!proto_metadata.IsInitialized()) {
    LOG(kError) << "Failed to construct pmid metadata.";
    ThrowError(CommonErrors::invalid_parameter);
  }
  pmid_name = PmidName(Identity(proto_metadata.pmid_name()));
  stored_count = proto_metadata.stored_count();
  stored_total_size = proto_metadata.stored_total_size();
  lost_count = proto_metadata.lost_count();
  lost_total_size = proto_metadata.lost_total_size();
  claimed_available_size = proto_metadata.claimed_available_size();
}

PmidManagerMetadata::PmidManagerMetadata(const PmidManagerMetadata& other)
    : pmid_name(other.pmid_name),
      stored_count(other.stored_count),
      stored_total_size(other.stored_total_size),
      lost_count(other.lost_count),
      lost_total_size(other.lost_total_size),
      claimed_available_size(other.claimed_available_size) {}

PmidManagerMetadata::PmidManagerMetadata(PmidManagerMetadata&& other)
    : pmid_name(std::move(other.pmid_name)),
      stored_count(std::move(other.stored_count)),
      stored_total_size(std::move(other.stored_total_size)),
      lost_count(std::move(other.lost_count)),
      lost_total_size(std::move(other.lost_total_size)),
      claimed_available_size(std::move(other.claimed_available_size)) {}

PmidManagerMetadata& PmidManagerMetadata::operator=(PmidManagerMetadata other) {
  using std::swap;
  swap(pmid_name, other.pmid_name);
  swap(stored_count, other.stored_count);
  swap(stored_total_size, other.stored_total_size);
  swap(lost_count, other.lost_count);
  swap(lost_total_size, other.lost_total_size);
  swap(claimed_available_size, other.claimed_available_size);
  return *this;
}

void PmidManagerMetadata::PutData(int32_t size) {
  stored_total_size += size;
  ++stored_count;
}

void PmidManagerMetadata::DeleteData(int32_t size) {
  stored_total_size -= size;
  --stored_count;

  if ((stored_total_size < 0) || (stored_count < 0)) {
    LOG(kError) << "invalid stored_total_size " << stored_total_size
                << " or stored_count " << stored_count;
    ThrowError(CommonErrors::invalid_parameter);
  }
}

void PmidManagerMetadata::HandleLostData(int32_t size) {
  DeleteData(size);
  lost_total_size += size;
  ++lost_count;
}

void PmidManagerMetadata::HandleFailure(int32_t size) {
  HandleLostData(size);
  claimed_available_size = 0;
}

void PmidManagerMetadata::SetAvailableSize(const int64_t& available_size) {
  claimed_available_size = available_size;
}


// BEFORE_RELEASE check if group can be deleted with below check
detail::GroupDbMetaDataStatus PmidManagerMetadata::GroupStatus() {
  return ((stored_count <= 0) && (claimed_available_size == 0) ?
      detail::GroupDbMetaDataStatus::kGroupEmpty : detail::GroupDbMetaDataStatus::kGroupNonEmpty);
}

std::string PmidManagerMetadata::Serialise() const {
  protobuf::PmidManagerMetadata proto_metadata;
  proto_metadata.set_pmid_name(pmid_name->string());
  proto_metadata.set_stored_count(stored_count);
  proto_metadata.set_stored_total_size(stored_total_size);
  proto_metadata.set_lost_count(lost_count);
  proto_metadata.set_lost_total_size(lost_total_size);
  proto_metadata.set_claimed_available_size(claimed_available_size);
  return proto_metadata.SerializeAsString();
}

bool operator==(const PmidManagerMetadata& lhs, const PmidManagerMetadata& rhs) {
  return lhs.pmid_name == rhs.pmid_name && lhs.stored_count == rhs.stored_count &&
         lhs.stored_total_size == rhs.stored_total_size && lhs.lost_count == rhs.lost_count &&
         lhs.lost_total_size == rhs.lost_total_size &&
         lhs.claimed_available_size == rhs.claimed_available_size;
}

}  // namespace vault
}  // namespace maidsafe
