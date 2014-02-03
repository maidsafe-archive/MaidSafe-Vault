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

#include "maidsafe/vault/pmid_manager/value.h"

#include "maidsafe/common/error.h"
#include "maidsafe/common/log.h"

#include "maidsafe/vault/pmid_manager/pmid_manager.pb.h"
#include "maidsafe/vault/utils.h"

namespace maidsafe {
namespace vault {

PmidManagerValue::PmidManagerValue() : size_(0) {}

PmidManagerValue::PmidManagerValue(int32_t size) : size_(size) {}

PmidManagerValue::PmidManagerValue(const std::string& serialised_pmid_manager_value)
    : size_(0) {
  protobuf::PmidManagerValue value_proto;
  if (!value_proto.ParseFromString(serialised_pmid_manager_value)) {
    LOG(kError) << "Failed to read or parse serialised pmid manager value.";
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
  } else {
    size_ = value_proto.size();
  }
}

PmidManagerValue::PmidManagerValue(PmidManagerValue&& other)
    : size_(std::move(other.size_)) {}

PmidManagerValue& PmidManagerValue::operator=(PmidManagerValue other) {
  swap(*this, other);
  return *this;
}

std::string PmidManagerValue::Serialise() const {
  protobuf::PmidManagerValue value_proto;
  value_proto.set_size(size_);
  return value_proto.SerializeAsString();
}

bool operator==(const PmidManagerValue& lhs, const PmidManagerValue& rhs) {
  return lhs.size() == rhs.size();
}

void swap(PmidManagerValue& lhs, PmidManagerValue& rhs) {
  using std::swap;
  swap(lhs.size_, rhs.size_);;
}

}  // namespace vault
}  // namespace maidsafe
