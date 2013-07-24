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

#include "maidsafe/vault/maid_manager/metadata.h"

#include <utility>


namespace maidsafe {

namespace vault {

MaidManagerMetadata::MaidManagerMetadata(const std::string& serialised_metadata_value) {
  protobuf::MaidManagerMetadata maid_manager_metadata_proto;
  if (!maid_manager_metadata_proto.ParseFromString(serialised_metadata_value)) {
    LOG(kError) << "Failed to read or parse serialised maid manager value";
    ThrowError(CommonErrors::parsing_error);
  } else {
    total_put_data_ = maid_manager_metadata_proto.total_put_data();
    for (auto index(0); index < maid_manager_metadata_proto.pmid_totals_size(); ++index) {
      pmid_totals_.emplace_back(PmidTotals(
          nfs::PmidRegistration::serialised_type(NonEmptyString(
              maid_manager_metadata_proto.pmid_totals(index).serialised_pmid_registration())),
          PmidManagerMetadata(maid_manager_metadata_proto.pmid_totals(index).pmid_record())));
    }
  }
}

MaidManagerMetadata::MaidManagerMetadata(const int64_t& total_put_data,
                                         const std::vector<PmidTotals>& pmid_totals)
    : total_put_data_(total_put_data),
      pmid_totals_(pmid_totals) {}

void MaidManagerMetadata::RegisterPmid(const nfs::PmidRegistration& pmid_registration) {
  auto itr(Find(pmid_registration.pmid_name()));
  if (itr == std::end(pmid_totals_)) {
    nfs::PmidRegistration::serialised_type serialised_pmid_registration(
        pmid_registration.Serialise());
    pmid_totals_.emplace_back(serialised_pmid_registration,
                              PmidManagerMetadata(pmid_registration.pmid_name()));
  }
}

void MaidManagerMetadata::UnregisterPmid(const nfs::PmidRegistration& pmid_registration) {
  auto itr(Find(pmid_registration.pmid_name()));
  if (itr != std::end(pmid_totals_))
    pmid_totals_.erase(itr);
}

void MaidManagerMetadata::UpdatePmidTotals(const PmidManagerMetadata& pmid_record) {
  auto itr(Find(pmid_record.pmid_name));
  if (itr == std::end(pmid_totals_))
    ThrowError(CommonErrors::no_such_element);
  (*itr).pmid_record = pmid_record;
}

std::string MaidManagerMetadata::Serialise() const {
  return ToProto().SerializeAsString();
}

protobuf::MaidManagerMetadata MaidManagerMetadata::ToProto() const {
  protobuf::MaidManagerMetadata maid_manager_metedata_proto;
  maid_manager_metedata_proto.set_total_put_data(total_put_data_);
  for (auto pmid_total : pmid_totals_) {
    auto pmid_total_proto(maid_manager_metedata_proto.add_pmid_totals());
    auto pmid_record(pmid_total_proto->mutable_pmid_record());
    *pmid_record = pmid_total.pmid_record.ToProtobuf();
    pmid_total_proto->set_serialised_pmid_registration(
        NonEmptyString(pmid_total.serialised_pmid_registration).string());
  }
  return maid_manager_metedata_proto;
}

std::vector<PmidTotals>::iterator MaidManagerMetadata::Find(const PmidName& pmid_name) {
  return std::find_if(std::begin(pmid_totals_),
                      std::end(pmid_totals_),
                      [&pmid_name](const PmidTotals& pmid_totals) {
                        return pmid_name == pmid_totals.pmid_record.pmid_name;
                      });
}

bool operator==(const MaidManagerMetadata& lhs, const MaidManagerMetadata& rhs) {
  return lhs.total_put_data_ == rhs.total_put_data_ &&
         lhs.pmid_totals_ == rhs.pmid_totals_;
}

void swap(MaidManagerMetadata& lhs, MaidManagerMetadata& rhs) {
  using std::swap;
  swap(lhs.total_put_data_, rhs.total_put_data_);
  swap(lhs.pmid_totals_, rhs.pmid_totals_);
}

}  // namespace vault

}  // namespace maidsafe
