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

#include "maidsafe/vault/pmid_manager/value.h"

#include "maidsafe/common/error.h"
#include "maidsafe/common/log.h"

#include "maidsafe/vault/pmid_manager/pmid_manager.pb.h"


namespace maidsafe {
namespace vault {

PmidManagerValue::PmidManagerValue() : size_(0) {}

PmidManagerValue::PmidManagerValue(int32_t size) : size_(size) {}

PmidManagerValue::PmidManagerValue(const std::string& serialised_pmid_manager_value)
    : size_(0) {
  protobuf::PmidManagerValue pmid_manager_value_proto;
  if (!pmid_manager_value_proto.ParseFromString(serialised_pmid_manager_value)) {
    LOG(kError) << "Failed to read or parse serialised pmid manager value.";
    ThrowError(CommonErrors::parsing_error);
  } else {
    size_ = pmid_manager_value_proto.size();
  }
}

std::string PmidManagerValue::Serialise() const {
  protobuf::PmidManagerValue pmid_manager_value_proto;
  pmid_manager_value_proto.set_size(size_);
  return pmid_manager_value_proto.SerializeAsString();
}

bool operator==(const PmidManagerValue& lhs, const PmidManagerValue& rhs) {
  return lhs.size() == rhs.size();
}

}  // namespace vault
}  // namespace maidsafe
