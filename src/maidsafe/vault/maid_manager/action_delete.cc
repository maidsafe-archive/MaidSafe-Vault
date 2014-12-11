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

#include "maidsafe/vault/maid_manager/action_delete.h"

#include "maidsafe/common/visualiser_log.h"

#include "maidsafe/vault/maid_manager/action_delete.pb.h"
#include "maidsafe/vault/maid_manager/value.h"

namespace maidsafe {

namespace vault {

ActionMaidManagerDelete::ActionMaidManagerDelete(nfs::MessageId message_id)
    : kMessageId(message_id) {}

ActionMaidManagerDelete::ActionMaidManagerDelete(const std::string& serialised_action)
    : kMessageId([&serialised_action]()->int32_t {
        protobuf::ActionMaidManagerDelete action_delete_proto;
        if (!action_delete_proto.ParseFromString(serialised_action))
          BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
        return action_delete_proto.message_id();
      }()) {}

ActionMaidManagerDelete::ActionMaidManagerDelete(const ActionMaidManagerDelete& other)
    : kMessageId(other.kMessageId) {}

ActionMaidManagerDelete::ActionMaidManagerDelete(ActionMaidManagerDelete&& other)
    :kMessageId(std::move(other.kMessageId)) {}

std::string ActionMaidManagerDelete::Serialise() const {
  protobuf::ActionMaidManagerDelete action_delete_proto;
  action_delete_proto.set_message_id(kMessageId);
  return action_delete_proto.SerializeAsString();
}

void ActionMaidManagerDelete::operator()(MaidManagerValue& /*value*/) const {
  /*if (!value)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::no_such_element));

  value.DeleteData(value->Delete());
  VLOG(nfs::Persona::kMaidManager, VisualiserAction::kDecreaseCount, value->count());
  assert(value->count() >= 0);
  if (value->count() == 0)
    return detail::DbAction::kDelete;
  else
    return detail::DbAction::kPut;*/
}


bool operator==(const ActionMaidManagerDelete& lhs, const ActionMaidManagerDelete& rhs) {
  return (lhs.kMessageId == rhs.kMessageId);
}

bool operator!=(const ActionMaidManagerDelete& lhs, const ActionMaidManagerDelete& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
