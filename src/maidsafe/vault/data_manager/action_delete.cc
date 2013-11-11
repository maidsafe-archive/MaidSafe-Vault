/*  Copyright 2013 MaidSafe.net limited

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

#include "maidsafe/vault/data_manager/action_delete.h"
#include "maidsafe/vault/data_manager/action_delete.pb.h"

#include "maidsafe/vault/data_manager/value.h"

namespace maidsafe {

namespace vault {

ActionDataManagerDelete::ActionDataManagerDelete() {}

ActionDataManagerDelete::ActionDataManagerDelete(const std::string& /*serialised_action*/) {}

std::string ActionDataManagerDelete::Serialise() const {
  protobuf::ActionDataManagerDelete action_delete_proto;
  return action_delete_proto.SerializeAsString();
}

detail::DbAction ActionDataManagerDelete::operator()(std::unique_ptr<DataManagerValue>& value) {
  if (value) {
    value->DecrementSubscribers();
    assert(value->Subscribers() >= 0);
    if (value->Subscribers() == 0)
      return detail::DbAction::kDelete;
    else
      return detail::DbAction::kPut;
  } else {
    ThrowError(CommonErrors::no_such_element);
    return detail::DbAction::kDelete;
  }
}

bool operator==(const ActionDataManagerDelete& /*lhs*/, const ActionDataManagerDelete& /*rhs*/) {
  return true;
}

bool operator!=(const ActionDataManagerDelete& lhs, const ActionDataManagerDelete& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
