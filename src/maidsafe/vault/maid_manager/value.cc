/*  Copyright 2012 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.maidsafe.net/licenses

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#include "maidsafe/vault/maid_manager/value.h"

#include <functional>
#include <limits>
#include <utility>

#include "maidsafe/common/error.h"
#include "maidsafe/common/log.h"
#include "maidsafe/common/visualiser_log.h"
#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/types.h"
#include "maidsafe/vault/maid_manager/maid_manager.pb.h"

namespace maidsafe {

namespace vault {

MaidManagerValue::MaidManagerValue(const std::string& serialised_maid_manager_value)
    : size_(0) {
  protobuf::MaidManagerValue maid_manager_value_proto;
  if (!maid_manager_value_proto.ParseFromString(serialised_maid_manager_value)) {
    LOG(kError) << "Failed to read or parse serialised maid manager value.";
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
  }
  size_ = maid_manager_value_proto.size();
  LOG(kVerbose) << "parsed size " << size_;
  if (size_ < 0) {
    LOG(kError) << "invalid size " << size_;
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::invalid_parameter));
  }
}

MaidManagerValue::MaidManagerValue() : size_(0) {}

MaidManagerValue::MaidManagerValue(MaidManagerValue&& other) MAIDSAFE_NOEXCEPT
    : size_(std::move(other.size_)) {}

MaidManagerValue& MaidManagerValue::operator=(MaidManagerValue other) {
  swap(*this, other);
  return *this;
}

std::string MaidManagerValue::Serialise() const {
  if (size_ == 0) {
    LOG(kError) << "MaidManagerValue::Serialise Cannot serialise if not a complete db value";
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::uninitialised));
  }

  protobuf::MaidManagerValue maid_manager_value_proto;
  maid_manager_value_proto.set_size(size_);
  return maid_manager_value_proto.SerializeAsString();
}

void MaidManagerValue::Put(int64_t size) {
//  VLOG(nfs::Persona::kMaidManager, VisualiserAction::kIncreaseCount, count_);
  size_ = size;
}

int64_t MaidManagerValue::Delete() {
  return size_;
}

void swap(MaidManagerValue& lhs, MaidManagerValue& rhs) {
  using std::swap;
  swap(lhs.size_, rhs.size_);
}

bool operator==(const MaidManagerValue& lhs, const MaidManagerValue& rhs) {
  return lhs.size() == rhs.size();
}

std::string MaidManagerValue::Print() const {
  std::stringstream stream;
  stream << "[size, " << size_ << "]";
  return stream.str();
}

}  // namespace vault

}  // namespace maidsafe
