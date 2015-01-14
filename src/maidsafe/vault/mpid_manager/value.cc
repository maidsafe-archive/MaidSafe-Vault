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
#include "maidsafe/vault/mpid_manager/mpid_manager.pb.h"

namespace maidsafe {

namespace vault {

MpidManagerValue::MpidManagerValue() : outbox_(), inbox_() {}

MpidManagerValue::MpidManagerValue(const MpidManagerValue& other)
    :  outbox_(other.outbox_), inbox_(other.inbox_) {}

MpidManagerValue::MpidManagerValue(MpidManagerValue&& other)
  : outbox_(std::move(other.outbox_)), inbox_(std::move(other.inbox_)) {}

MpidManagerValue& MpidManagerValue::operator=(MpidManagerValue other) {
  swap(*this, other);
  return *this;
}

MpidManagerValue::MpidManagerValue(const std::string& serialised_value) {
  protobuf::MpidManagerValue proto;
  if (!proto.ParseFromString(serialised_value))
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));

  for (const auto& outbox_entry : proto.serialised_outbox_entry())
    outbox_.emplace_back(nfs_vault::MpidMessage(outbox_entry));

  for (const auto& inbox_entry : proto.serialised_inbox_entry())
    inbox_.emplace_back(nfs_vault::MpidMessageAlert(inbox_entry));
}

std::string MpidManagerValue::Serialise() const {
  protobuf::MpidManagerValue proto;

  for (const auto& outbox_entry : outbox_)
    proto.add_serialised_outbox_entry(outbox_entry.Serialise());

  for (const auto& inbox_entry : inbox_)
    proto.add_serialised_inbox_entry(inbox_entry.Serialise());

  return std::string();
}

MpidManagerValue MpidManagerValue::Resolve(const std::vector<MpidManagerValue>& /*values*/) {
  MpidManagerValue value;
  return value;
}

void swap(MpidManagerValue& lhs, MpidManagerValue& rhs) {
  using std::swap;
  swap(lhs.outbox_, rhs.outbox_);
  swap(lhs.inbox_, rhs.inbox_);
}

bool operator==(const MpidManagerValue& lhs, const MpidManagerValue& rhs) {
  return lhs.outbox_.size() == rhs.outbox_.size() &&
         lhs.inbox_.size() == rhs.inbox_.size() &&
         std::equal(lhs.outbox_.begin(), lhs.outbox_.end(), rhs.outbox_.begin()) &&
         std::equal(lhs.inbox_.begin(), lhs.inbox_.end(), rhs.inbox_.begin());
}

}  // namespace vault

}  // namespace maidsafe
