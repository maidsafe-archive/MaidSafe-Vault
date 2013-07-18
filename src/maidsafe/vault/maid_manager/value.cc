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

#include "maidsafe/vault/maid_manager/value.h"

#include <string>

#include "maidsafe/vault/maid_manager/maid_manager.pb.h"

namespace maidsafe {

namespace vault {

MaidManagerValue::MaidManagerValue(const serialised_type& serialised_metadata_value) {
  protobuf::MaidManagerDbValue maid_manager_value_proto;
  if (!maid_manager_value_proto.ParseFromString(serialised_metadata_value->string())) {
    LOG(kError) << "Failed to read or parse serialised maid manager value";
    ThrowError(CommonErrors::parsing_error);
  } else {
    count_ = maid_manager_value_proto.count();
    cost_ = maid_manager_value_proto.average_cost();
  }
}

MaidManagerValue::MaidManagerValue()
    : count_(0),
      cost_(0) {}

void MaidManagerValue::Put(const int32_t& cost) {
  count_++;
  cost_ += cost;
}

void MaidManagerValue::Delete(const int32_t& cost) {
  if ((count_ == 0) || (cost_ - cost <= 0))
    ThrowError(CommonErrors::unknown);  // Unabel to delete
  count_--;
  cost_ -= cost;
}

MaidManagerValue::serialised_type MaidManagerValue::Serialise() const {
  if ((count_ == 0) || (cost_ == 0))
    ThrowError(CommonErrors::uninitialised);  // Cannot serialise if not a complete db value

  protobuf::MaidManagerDbValue maid_manager_value_proto;
  maid_manager_value_proto.set_count(count_);
  maid_manager_value_proto.set_average_cost(cost_);
  return serialised_type(NonEmptyString(maid_manager_value_proto.SerializeAsString()));
}

bool operator==(const MaidManagerValue& lhs, const MaidManagerValue& rhs) {
  return lhs.count_ == rhs.count_ && lhs.cost_ == rhs.cost_;
}

}  // namespace vault

}  // namespace maidsafe
