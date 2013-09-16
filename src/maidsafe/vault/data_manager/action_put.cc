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

#include "maidsafe/vault/data_manager/action_put.h"
#include "maidsafe/vault/data_manager/action_put.pb.h"

#include "maidsafe/vault/pmid_manager/value.h"

namespace maidsafe {
namespace vault {

ActionDataManagerPut::ActionDataManagerPut(const PmidName& pmid_name, const uint32_t& size)
    : kSize(size),
      kPmidName(pmid_name) {}

ActionDataManagerPut::ActionDataManagerPut(
    const std::string& serialised_action)
    : kSize([&]()->int32_t {
              protobuf::ActionDataManagerPut action_put_proto;
              action_put_proto.ParseFromString(serialised_action);
              return action_put_proto.size();
            }()),
      kDataName([&]()->DataNameVariant {
                  protobuf::ActionDataManagerPut action_put_proto;
                  action_put_proto.ParseFromString(serialised_action);
                  return GetDataNameVariant(static_cast<DataTagValue>(action_put_proto.data_type()),
                                            Identity(action_put_proto.data_name()));
      }()) {}

ActionDataManagerPut::ActionDataManagerPut(
    const ActionDataManagerPut& other)
    : kSize(other.kSize),
      kDataName(other.kDataName) {}

ActionDataManagerPut::ActionDataManagerPut(
    ActionDataManagerPut&& other)
    : kSize(std::move(other.kSize)),
      kDataName(std::move(other.kDataName)) {}

std::string ActionDataManagerPut::Serialise() const {
  protobuf::ActionDataManagerPut action_put_proto;
  action_put_proto.set_data_name((boost::apply_visitor(GetIdentityVisitor(), kDataName)).string());
  action_put_proto.set_data_type(static_cast<int32_t>(boost::apply_visitor(GetTagValueVisitor(),
                                                                           kDataName)));
  action_put_proto.set_size(kSize);
  return action_put_proto.SerializeAsString();
}

void ActionDataManagerPut::operator()(boost::optional<DataManagerValue>& value) {
  if (value) {
    value->IncrementSubscribers();
    value->AddPmid(kPmidName);
  } else {
    value.reset(DataManagerValue(kSize, kPmidName));
    value->IncrementSubscribers();
    value->AddPmid(kPmidName);
  }
}

bool operator==(const ActionDataManagerPut& lhs,
                const ActionDataManagerPut& rhs) {
  return lhs.kDataName == rhs.kDataName && lhs.kSize == rhs.kSize;
}

bool operator!=(const ActionDataManagerPut& lhs,
                const ActionDataManagerPut& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault
}  // namespace maidsafe
