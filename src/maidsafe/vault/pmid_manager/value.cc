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

#include <string>

#include "maidsafe/vault/utils.h"

namespace maidsafe {
namespace vault {

PmidManagerValue::PmidManagerValue(const serialised_type& serialised_value)
  : value() {
  protobuf::PmidManagerValue value_proto;
  if (!value_proto.ParseFromString(serialised_value->string())) {
    LOG(kError) << "Failed to read or parse serialised value";
    ThrowError(CommonErrors::parsing_error);
  } else {
    size = value_proto.size();
  }
}

PmidManagerValue::PmidManagerValue(int size)
  : size(size) {}

bool operator==(const PmidManagerValue& lhs, const PmidManagerValue& rhs) {
  return lhs.size == rhs.size;
}

PmidManagerValue::serialised_type PmidManagerValue::Serialise() const {
  protobuf::PmidManagerValue value_proto;
  value_proto.set_size(size);
  assert(value_proto.IsInitialized());
  return serialised_type(NonEmptyString(value_proto.SerializeAsString()));
}

}  // namespace vault
}  // namespace maidsafe
