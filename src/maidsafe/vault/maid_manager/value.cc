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
#include "maidsafe/common/on_scope_exit.h"

#include "maidsafe/vault/maid_manager/maid_manager.pb.h"


namespace maidsafe {

namespace vault {

MaidManagerValue::MaidManagerValue(const std::string& serialised_maid_manager_value)
    : count_(0),
      cost_(0) {
  protobuf::MaidManagerValue maid_manager_value_proto;
  if (!maid_manager_value_proto.ParseFromString(serialised_maid_manager_value)) {
    LOG(kError) << "Failed to read or parse serialised maid manager value.";
    ThrowError(CommonErrors::parsing_error);
  } else {
    count_ = maid_manager_value_proto.count();
    cost_ = maid_manager_value_proto.average_cost();
  }
}

MaidManagerValue::MaidManagerValue() : count_(0), cost_(0) {}

std::string MaidManagerValue::Serialise() const {
  if (count_ == 0 || cost_ == 0)
    ThrowError(CommonErrors::uninitialised);

  protobuf::MaidManagerValue maid_manager_value_proto;
  maid_manager_value_proto.set_count(count_);
  maid_manager_value_proto.set_average_cost(cost_);
  return maid_manager_value_proto.SerializeAsString();
}

void MaidManagerValue::Put(int32_t cost) {
  ++count_;
  cost_ += cost;
}

void MaidManagerValue::Delete(int32_t cost) {
  auto reset_values([this](int32_t original_count, int32_t original_cost) {
      count_ = original_count;
      cost_ = original_cost;
  });
  on_scope_exit strong_guarantee(std::bind(reset_values, count_, cost_));

  --count_;
  cost_ -= cost;

  if (count_ < 0 || cost_ < 0)
    ThrowError(CommonErrors::invalid_parameter);

  strong_guarantee.Release();
}

bool operator==(const MaidManagerValue& lhs, const MaidManagerValue& rhs) {
  return lhs.count() == rhs.count() && lhs.cost() == rhs.cost();
}

}  // namespace vault

}  // namespace maidsafe
