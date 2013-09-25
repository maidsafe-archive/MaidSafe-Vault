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

#include "maidsafe/vault/data_manager/action_put.h"
#include "maidsafe/vault/data_manager/action_put.pb.h"

#include "maidsafe/vault/pmid_manager/value.h"

namespace maidsafe {
namespace vault {

ActionDataManagerPut::ActionDataManagerPut(const std::string& /*serialised_action*/) {
}

ActionDataManagerPut::ActionDataManagerPut(const ActionDataManagerPut& /*other*/) {
}

ActionDataManagerPut::ActionDataManagerPut(ActionDataManagerPut&& /*other*/) {

}

std::string ActionDataManagerPut::Serialise() const {
  protobuf::ActionDataManagerPut action_put_proto;
  return action_put_proto.SerializeAsString();
}

void ActionDataManagerPut::operator()(boost::optional<DataManagerValue>& value) {
  if (value) {
    value->IncrementSubscribers();
  } else {
    ThrowError(CommonErrors::no_such_element);
  }
}

bool operator==(const ActionDataManagerPut& /*lhs*/, const ActionDataManagerPut& /*rhs*/) {
  return true;
}

bool operator!=(const ActionDataManagerPut& lhs,
                const ActionDataManagerPut& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault
}  // namespace maidsafe
