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

#include "maidsafe/vault/maid_manager/non_persistent_value.h"

#include <string>

#include "maidsafe/vault/maid_manager/maid_manager.pb.h"

namespace maidsafe {

namespace vault {

MaidManagerNonPersistentValue::MaidManagerNonPersistentValue(
    const serialised_type& serialised_metadata_value) {
  protobuf::MaidManagerNonPersistentValue maid_manager_value_proto;
  if (!maid_manager_value_proto.ParseFromString(serialised_metadata_value->string())) {
    LOG(kError) << "Failed to read or parse serialised maid manager value";
    ThrowError(CommonErrors::parsing_error);
  } else {
    for (auto index(0); index < maid_manager_value_proto.pmid_totals_size(); ++index) {
      pmid_totals_.push_back(PmidTotals(
          nfs::PmidRegistration::serialised_type(NonEmptyString(
              maid_manager_value_proto.pmid_totals(index).serialised_pmid_registration())),
          PmidRecord(maid_manager_value_proto.pmid_totals(index).pmid_record())));
    }
  }
}

MaidManagerNonPersistentValue::MaidManagerNonPersistentValue(
    const int64_t& total_put_data, const std::vector<PmidTotals> pmid_totals)
    : total_put_data_(total_put_data),
      pmid_totals_(pmid_totals) {}

void MaidManagerNonPersistentValue::RegisterPmid(const nfs::PmidRegistration& pmid_registration) {
  auto itr(Find(pmid_registration.pmid_name()));
  if (itr == std::end(pmid_totals_)) {
    nfs::PmidRegistration::serialised_type serialised_pmid_registration(
        pmid_registration.Serialise());
    pmid_totals_.emplace_back(serialised_pmid_registration,
                              PmidRecord(pmid_registration.pmid_name()));
  }
}

void MaidManagerNonPersistentValue::UnregisterPmid(const PmidName& pmid_name) {
  auto itr(Find(pmid_name));
  if (itr != std::end(pmid_totals_))
    pmid_totals_.erase(itr);
}

void MaidManagerNonPersistentValue::UpdatePmidTotals(const PmidRecord& pmid_record) {
  auto itr(Find(pmid_record.pmid_name));
  if (itr == std::end(pmid_totals_))
    ThrowError(CommonErrors::no_such_element);
  (*itr).pmid_record = pmid_record;
}

MaidManagerNonPersistentValue::serialised_type MaidManagerNonPersistentValue::Serialise() const {
  return serialised_type(NonEmptyString("fix me"));
}

std::vector<PmidTotals>::iterator MaidManagerNonPersistentValue::Find(const PmidName& pmid_name) {
  return std::find_if(std::begin(pmid_totals_),
                      std::end(pmid_totals_),
                      [&pmid_name](const PmidTotals& pmid_totals) {
                        return pmid_name == pmid_totals.pmid_record.pmid_name;
                      });
}

//bool operator==(const MaidManagerNonPersistentValue& lhs, const MaidManagerNonPersistentValue& rhs) {
//  return lhs.total_put_data_ == rhs.total_put_data_ &&
//          lhs.pmid_totals_ == rhs.pmid_totals_;
//}

}  // namespace vault

}  // namespace maidsafe
