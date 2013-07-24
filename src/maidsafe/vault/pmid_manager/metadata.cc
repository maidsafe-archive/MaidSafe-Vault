/* Copyright 2012 MaidSafe.net limited

This MaidSafe Software is licensed under the MaidSafe.net Commercial License, version 1.0 or later,
and The General Public License (GPL), version 3. By contributing code to this project You agree to
the terms laid out in the MaidSafe Contributor Agreement, version 1.0, found in the root directory
of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available at:

http://www.novinet.com/license

Unless required by applicable law or agreed to in writing, software distributed under the License is
distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.
*/

#include "maidsafe/vault/pmid_manager/metadata.h"

#include "maidsafe/common/log.h"

#include "maidsafe/vault/pmid_manager/pmid_manager.pb.h"


namespace maidsafe {
namespace vault {

PmidManagerMetadata::PmidManagerMetadata()
    : pmid_name(),
      stored_count(0),
      stored_total_size(0),
      lost_count(0),
      lost_total_size(0),
      claimed_available_size(0) {}

PmidManagerMetadata::PmidManagerMetadata(const PmidName& pmid_name_in)
    : pmid_name(pmid_name_in),
      stored_count(0),
      stored_total_size(0),
      lost_count(0),
      lost_total_size(0),
      claimed_available_size(0) {}

PmidManagerMetadata::PmidManagerMetadata(const serialised_type& serialised_metadata)
    : pmid_name(),
      stored_count(0),
      stored_total_size(0),
      lost_count(0),
      lost_total_size(0),
      claimed_available_size(0) {
  protobuf::PmidManagerMetadata proto_metadata;
  if (!proto_metadata.ParseFromString(serialised_metadata->string())) {
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

PmidManagerMetadata::serialised_type PmidManagerMetadata::Serialise() const {
  protobuf::PmidManagerMetadata proto_metadata;
  proto_metadata.set_pmid_name(pmid_name->string());
  proto_metadata.set_stored_count(stored_count);
  proto_metadata.set_stored_total_size(stored_total_size);
  proto_metadata.set_lost_count(lost_count);
  proto_metadata.set_lost_total_size(lost_total_size);
  proto_metadata.set_claimed_available_size(claimed_available_size);
  return serialised_type(NonEmptyString(proto_metadata.SerializeAsString()));
}

bool operator==(const PmidManagerMetadata& lhs, const PmidManagerMetadata& rhs) {
  return lhs.pmid_name == rhs.pmid_name &&
         lhs.stored_count == rhs.stored_count &&
         lhs.stored_total_size == rhs.stored_total_size &&
         lhs.lost_count == rhs.lost_count &&
         lhs.lost_total_size == rhs.lost_total_size &&
         lhs.claimed_available_size == rhs.claimed_available_size;
}

}  // namespace vault
}  // namespace maidsafe
