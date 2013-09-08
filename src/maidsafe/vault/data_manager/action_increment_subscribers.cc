/*  Copyright 2013 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.novinet.com/license

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#include "maidsafe/vault/data_manager/action_increment_subscribers.h"
#include "maidsafe/vault/data_manager/action_increment_subscribers.pb.h"

#include "maidsafe/vault/pmid_manager/value.h"

namespace maidsafe {
namespace vault {

ActionDataManagerIncrementSubscribers::ActionDataManagerIncrementSubscribers(
    const PmidName& pmid_name, const uint32_t& size)
    : kSize(size),
      kPmidName(pmid_name) {}

ActionDataManagerIncrementSubscribers::ActionDataManagerIncrementSubscribers(
    const std::string& serialised_action)
    : kSize([&]()->int32_t {
              protobuf::ActionDataManagerIncrementSubscribers action_increment_subscribers_proto;
              action_increment_subscribers_proto.ParseFromString(serialised_action);
              return action_increment_subscribers_proto.size();
            }()),
      kPmidName([&]()->Identity {
                  protobuf::ActionDataManagerIncrementSubscribers action_increment_subscribers_proto;
                  action_increment_subscribers_proto.ParseFromString(serialised_action);
                  return Identity(action_increment_subscribers_proto.pmid_name());
      }()) {}

ActionDataManagerIncrementSubscribers::ActionDataManagerIncrementSubscribers(
    const ActionDataManagerIncrementSubscribers& other)
    : kSize(other.kSize),
      kPmidName(other.kPmidName) {}

ActionDataManagerIncrementSubscribers::ActionDataManagerIncrementSubscribers(
    ActionDataManagerIncrementSubscribers&& other)
    : kSize(std::move(other.kSize)),
      kPmidName(std::move(other.kPmidName)) {}

std::string ActionDataManagerIncrementSubscribers::Serialise() const {
  protobuf::ActionDataManagerIncrementSubscribers action_increment_subscribers_proto;
  action_increment_subscribers_proto.set_pmid_name(kPmidName->string());
  action_increment_subscribers_proto.set_size(kSize);
  return action_increment_subscribers_proto.SerializeAsString();
}

void ActionDataManagerIncrementSubscribers::operator()(boost::optional<DataManagerValue>& value) {
  if (value)
    value->IncrementSubscribers();
  else
    value.reset(DataManagerValue(kPmidName, kSize));
}

bool operator==(const ActionDataManagerIncrementSubscribers& lhs,
                const ActionDataManagerIncrementSubscribers& rhs) {
  return lhs.kPmidName == rhs.kPmidName && lhs.kSize == rhs.kSize;
}

bool operator!=(const ActionDataManagerIncrementSubscribers& lhs,
                const ActionDataManagerIncrementSubscribers& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault
}  // namespace maidsafe
