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

#include <functional>

#include "maidsafe/common/error.h"
#include "maidsafe/common/log.h"

#include "maidsafe/vault/maid_manager/maid_manager.pb.h"


namespace maidsafe {

namespace vault {

MaidManagerValue::MaidManagerValue(const std::string& serialised_maid_manager_value)
    : count_(0),
      total_cost_(0) {
  protobuf::MaidManagerValue maid_manager_value_proto;
  if (!maid_manager_value_proto.ParseFromString(serialised_maid_manager_value)) {
    LOG(kError) << "Failed to read or parse serialised maid manager value.";
    ThrowError(CommonErrors::parsing_error);
  } else {
    count_ = maid_manager_value_proto.count();
    total_cost_ = maid_manager_value_proto.total_cost();
  }
}

MaidManagerValue::MaidManagerValue() : count_(0), total_cost_(0) {}

std::string MaidManagerValue::Serialise() const {
  if (count_ == 0 || total_cost_ == 0)
    ThrowError(CommonErrors::uninitialised);

  protobuf::MaidManagerValue maid_manager_value_proto;
  maid_manager_value_proto.set_count(count_);
  maid_manager_value_proto.set_total_cost(total_cost_);
  return maid_manager_value_proto.SerializeAsString();
}

void MaidManagerValue::Put(int32_t cost) {
  ++count_;
  total_cost_ += cost;
}

void MaidManagerValue::Delete() {
  if (count_ <= 0 || total_cost_ < 0)
    ThrowError(CommonErrors::unknown);
  auto average_cost(total_cost_ / count_);
  --count_;
  total_cost_ -= average_cost;
}

bool operator==(const MaidManagerValue& lhs, const MaidManagerValue& rhs) {
  return lhs.count() == rhs.count() && lhs.total_cost() == rhs.total_cost();
}

}  // namespace vault

}  // namespace maidsafe
