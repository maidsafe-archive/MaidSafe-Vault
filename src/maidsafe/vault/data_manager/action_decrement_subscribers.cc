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

#include "maidsafe/vault/data_manager/action_decrement_subscribers.h"
#include "maidsafe/vault/data_manager/action_decrement_subscribers.pb.h"

#include "maidsafe/vault/data_manager/value.h"


namespace maidsafe {
namespace vault {

const nfs::MessageAction ActionDataManagerDecrementSubscribers::kActionId;

std::string ActionDataManagerDecrementSubscribers::Serialise() const {
  protobuf::ActionDataManagerDecrementSubscribers action_decrement_subscribers_proto;
  return action_decrement_subscribers_proto.SerializeAsString();
}

void ActionDataManagerDecrementSubscribers::operator()(
    boost::optional<DataManagerValue>& value) {
  if (value)
    value->DecrementSubscribers();
  else
    ThrowError(CommonErrors::invalid_parameter);
}

bool operator==(const ActionDataManagerDecrementSubscribers& /*lhs*/,
                const ActionDataManagerDecrementSubscribers& /*rhs*/) {
  return true;
}

bool operator!=(const ActionDataManagerDecrementSubscribers& lhs,
                const ActionDataManagerDecrementSubscribers& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
