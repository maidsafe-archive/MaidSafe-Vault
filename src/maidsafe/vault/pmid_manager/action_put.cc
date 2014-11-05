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

#include "maidsafe/vault/pmid_manager/action_put.h"
#include "maidsafe/vault/pmid_manager/action_put.pb.h"

#include "maidsafe/vault/pmid_manager/value.h"

namespace maidsafe {

namespace vault {

ActionPmidManagerPut::ActionPmidManagerPut(int32_t size, nfs::MessageId message_id)
    : kSize(size), kMessageId(message_id) {}

ActionPmidManagerPut::ActionPmidManagerPut(const std::string& serialised_action)
  : kSize([&serialised_action]()->int32_t {
            protobuf::ActionPmidManagerPut action_put_proto;
            if (!action_put_proto.ParseFromString(serialised_action))
              BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
            return action_put_proto.size();
          }()),
    kMessageId([&serialised_action]()->int32_t {
                 protobuf::ActionPmidManagerPut action_put_proto;
                 action_put_proto.ParseFromString(serialised_action);
                 return action_put_proto.message_id();
               }()) {}

ActionPmidManagerPut::ActionPmidManagerPut(const ActionPmidManagerPut& other)
    : kSize(other.kSize), kMessageId(other.kMessageId) {}

ActionPmidManagerPut::ActionPmidManagerPut(ActionPmidManagerPut&& other)
    : kSize(std::move(other.kSize)), kMessageId(other.kMessageId) {}

std::string ActionPmidManagerPut::Serialise() const {
  protobuf::ActionPmidManagerPut action_put_proto;
  action_put_proto.set_size(kSize);
  action_put_proto.set_message_id(kMessageId);
  return action_put_proto.SerializeAsString();
}

void ActionPmidManagerPut::operator()(PmidManagerValue& value) {
  value.PutData(kSize);
}

bool operator==(const ActionPmidManagerPut& lhs, const ActionPmidManagerPut& rhs) {
  return lhs.kSize == rhs.kSize;
}

bool operator!=(const ActionPmidManagerPut& lhs, const ActionPmidManagerPut& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
