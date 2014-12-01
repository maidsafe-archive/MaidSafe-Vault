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

#include "maidsafe/common/error.h"
#include "maidsafe/common/data_types/data_name_variant.h"

#include "maidsafe/vault/data_manager/action_put.pb.h"
#include "maidsafe/vault/data_manager/value.h"

namespace maidsafe {

namespace vault {

ActionDataManagerPut::ActionDataManagerPut(uint64_t size, nfs::MessageId message_id)
    : kSize(size), kMessageId(message_id) {}

ActionDataManagerPut::ActionDataManagerPut(const std::string& serialised_action)
    : kSize([&serialised_action]()->uint64_t {
        protobuf::ActionDataManagerPut action_put_proto;
        if (!action_put_proto.ParseFromString(serialised_action))
          BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
        return action_put_proto.size();
      }()),
      kMessageId([&serialised_action]()->nfs::MessageId {
        protobuf::ActionDataManagerPut action_put_proto;
        if (!action_put_proto.ParseFromString(serialised_action))
          BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
        return nfs::MessageId(action_put_proto.message_id());
      }()) {}

ActionDataManagerPut::ActionDataManagerPut(const ActionDataManagerPut& other)
    : kSize(other.kSize), kMessageId(other.kMessageId) {}

ActionDataManagerPut::ActionDataManagerPut(ActionDataManagerPut&& other)
    : kSize(std::move(other.kSize)), kMessageId(std::move(other.kMessageId)) {}

std::string ActionDataManagerPut::Serialise() const {
  protobuf::ActionDataManagerPut action_put_proto;
  action_put_proto.set_size(kSize);
  action_put_proto.set_message_id(kMessageId);
  return action_put_proto.SerializeAsString();
}

detail::DbAction ActionDataManagerPut::operator()(std::unique_ptr<DataManagerValue>& value) {
  if (!value)
    value.reset(new DataManagerValue(kSize));
  else
    assert(value->chunk_size() == kSize);
  return detail::DbAction::kPut;
}

bool operator==(const ActionDataManagerPut& lhs, const ActionDataManagerPut& rhs) {
  return lhs.kSize == rhs.kSize && lhs.kMessageId == rhs.kMessageId;
}

bool operator!=(const ActionDataManagerPut& lhs, const ActionDataManagerPut& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
