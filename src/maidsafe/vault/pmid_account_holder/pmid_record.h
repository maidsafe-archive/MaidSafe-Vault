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

#ifndef MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_RECORD_H_
#define MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_RECORD_H_

#include <cstdint>

#include "maidsafe/common/tagged_value.h"
#include "maidsafe/common/types.h"

#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

namespace protobuf { class PmidRecord; }

struct PmidRecord {
 public:
  typedef TaggedValue<NonEmptyString, struct SerialisedPmidRecordTag> serialised_type;

  PmidRecord();
  explicit PmidRecord(const PmidName& pmid_name_in);
  explicit PmidRecord(const protobuf::PmidRecord& proto_pmid_record);
  protobuf::PmidRecord ToProtobuf() const;
  explicit PmidRecord(const serialised_type& serialised_pmid_record);
  serialised_type Serialise() const;
  PmidRecord(const PmidRecord& other);
  PmidRecord(PmidRecord&& other);
  PmidRecord& operator=(PmidRecord other);

  PmidName pmid_name;
  int64_t stored_count;
  int64_t stored_total_size;
  int64_t lost_count;
  int64_t lost_total_size;
  int64_t claimed_available_size;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_PMID_RECORD_H_
