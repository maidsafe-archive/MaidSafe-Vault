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

#include "maidsafe/vault/mpid_manager/value.h"

#include <utility>
#include <limits>

#include "maidsafe/vault/utils.h"
#include "maidsafe/routing/parameters.h"
// #include "maidsafe/vault/mpid_manager/mpid_manager.pb.h"

namespace maidsafe {

namespace vault {

MpidManagerValue::MpidManagerValue() {}

MpidManagerValue::MpidManagerValue(const MpidManagerValue& /*other*/) {}

MpidManagerValue::MpidManagerValue(MpidManagerValue&& /*other*/) {}

MpidManagerValue& MpidManagerValue::operator=(MpidManagerValue other) {
  swap(*this, other);
  return *this;
}

MpidManagerValue::MpidManagerValue(const std::string& /*serialised_value*/) {}


std::string MpidManagerValue::Serialise() const {
  return std::string();
}

MpidManagerValue MpidManagerValue::Resolve(const std::vector<MpidManagerValue>& /*values*/) {
  MpidManagerValue value;
  return value;
}

void swap(MpidManagerValue& /*lhs*/, MpidManagerValue& /*rhs*/) {
  using std::swap;
}

bool operator==(const MpidManagerValue& /*lhs*/, const MpidManagerValue& /*rhs*/) {
  return true;
}

}  // namespace vault

}  // namespace maidsafe
