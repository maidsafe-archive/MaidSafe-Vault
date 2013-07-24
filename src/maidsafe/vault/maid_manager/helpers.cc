/* Copyright 2013 MaidSafe.net limited

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

#include "maidsafe/vault/maid_manager/helpers.h"


namespace maidsafe {

namespace vault {

template<>
void PmidRegistrationOp::SetPublicFob<passport::PublicMaid>(
    std::unique_ptr<passport::PublicMaid>&& pub_maid) {
  public_maid = std::move(pub_maid);
}

template<>
void PmidRegistrationOp::SetPublicFob<passport::PublicPmid>(
    std::unique_ptr<passport::PublicPmid>&& pub_pmid) {
  public_pmid = std::move(pub_pmid);
}



PmidTotals::PmidTotals() : serialised_pmid_registration(), pmid_record() {}

PmidTotals::PmidTotals(
    const nfs::PmidRegistration::serialised_type& serialised_pmid_registration_in)
        : serialised_pmid_registration(serialised_pmid_registration_in),
          pmid_record() {}

PmidTotals::PmidTotals(
    const nfs::PmidRegistration::serialised_type& serialised_pmid_registration_in,
    const PmidManagerMetadata& pmid_record_in)
        : serialised_pmid_registration(serialised_pmid_registration_in),
          pmid_record(pmid_record_in) {}

PmidTotals::PmidTotals(const PmidTotals& other)
    : serialised_pmid_registration(other.serialised_pmid_registration),
      pmid_record(other.pmid_record) {}

PmidTotals::PmidTotals(PmidTotals&& other)
    : serialised_pmid_registration(std::move(other.serialised_pmid_registration)),
      pmid_record(std::move(other.pmid_record)) {}

PmidTotals& PmidTotals::operator=(PmidTotals other) {
  using std::swap;
  swap(serialised_pmid_registration, other.serialised_pmid_registration);
  swap(pmid_record, other.pmid_record);
  return *this;
}


bool operator==(const PmidTotals& lhs, const PmidTotals& rhs) {
  return (lhs.pmid_record == rhs.pmid_record) &&
          (lhs.serialised_pmid_registration == rhs.serialised_pmid_registration);
}

}  // namespace vault

}  // namespace maidsafe
