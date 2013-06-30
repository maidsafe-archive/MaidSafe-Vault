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

#include "maidsafe/vault/pmid_manager/pmid_record.h"

#include "maidsafe/common/log.h"

#include "maidsafe/vault/pmid_manager/pmid_manager.pb.h"


namespace maidsafe {

namespace vault {

PmidRecord::PmidRecord()
    : pmid_name(),
      stored_count(0),
      stored_total_size(0),
      lost_count(0),
      lost_total_size(0),
      claimed_available_size(0) {}

PmidRecord::PmidRecord(const PmidName& pmid_name_in)
    : pmid_name(pmid_name_in),
      stored_count(0),
      stored_total_size(0),
      lost_count(0),
      lost_total_size(0),
      claimed_available_size(0) {}

PmidRecord::PmidRecord(const protobuf::PmidRecord& proto_pmid_record)
    : pmid_name(),
      stored_count(0),
      stored_total_size(0),
      lost_count(0),
      lost_total_size(0),
      claimed_available_size(0) {
  if (!proto_pmid_record.IsInitialized()) {
    LOG(kError) << "Failed to construct pmid_record.";
    ThrowError(CommonErrors::invalid_parameter);
  }
  pmid_name = PmidName(Identity(proto_pmid_record.pmid_name()));
  stored_count = proto_pmid_record.stored_count();
  stored_total_size = proto_pmid_record.stored_total_size();
  lost_count = proto_pmid_record.lost_count();
  lost_total_size = proto_pmid_record.lost_total_size();
  claimed_available_size = proto_pmid_record.claimed_available_size();
}

protobuf::PmidRecord PmidRecord::ToProtobuf() const {
  protobuf::PmidRecord proto_pmid_record;
  proto_pmid_record.set_pmid_name(pmid_name->string());
  proto_pmid_record.set_stored_count(stored_count);
  proto_pmid_record.set_stored_total_size(stored_total_size);
  proto_pmid_record.set_lost_count(lost_count);
  proto_pmid_record.set_lost_total_size(lost_total_size);
  proto_pmid_record.set_claimed_available_size(claimed_available_size);
  return proto_pmid_record;
}

PmidRecord::PmidRecord(const serialised_type& serialised_pmid_record)
    : pmid_name(),
      stored_count(0),
      stored_total_size(0),
      lost_count(0),
      lost_total_size(0),
      claimed_available_size(0) {
  protobuf::PmidRecord proto_pmid_record;
  if (!proto_pmid_record.ParseFromString(serialised_pmid_record->string())) {
    LOG(kError) << "Failed to parse pmid_record.";
    ThrowError(CommonErrors::parsing_error);
  }
  *this = PmidRecord(proto_pmid_record);
}

PmidRecord::serialised_type PmidRecord::Serialise() const {
  auto proto_pmid_record(ToProtobuf());
  return serialised_type(NonEmptyString(proto_pmid_record.SerializeAsString()));
}

PmidRecord::PmidRecord(const PmidRecord& other)
    : pmid_name(other.pmid_name),
      stored_count(other.stored_count),
      stored_total_size(other.stored_total_size),
      lost_count(other.lost_count),
      lost_total_size(other.lost_total_size),
      claimed_available_size(other.claimed_available_size) {}

PmidRecord::PmidRecord(PmidRecord&& other)
    : pmid_name(std::move(other.pmid_name)),
      stored_count(std::move(other.stored_count)),
      stored_total_size(std::move(other.stored_total_size)),
      lost_count(std::move(other.lost_count)),
      lost_total_size(std::move(other.lost_total_size)),
      claimed_available_size(std::move(other.claimed_available_size)) {}

PmidRecord& PmidRecord::operator=(PmidRecord other) {
  using std::swap;
  swap(pmid_name, other.pmid_name);
  swap(stored_count, other.stored_count);
  swap(stored_total_size, other.stored_total_size);
  swap(lost_count, other.lost_count);
  swap(lost_total_size, other.lost_total_size);
  swap(claimed_available_size, other.claimed_available_size);
  return *this;
}

}  // namespace vault

}  // namespace maidsafe
