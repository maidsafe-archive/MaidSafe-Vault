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


namespace maidsafe {
namespace vault {

const nfs::MessageAction ActionPmidManagerPut::kActionId(nfs::MessageAction::kPutRequest);

template<typename Data>
ActionPmidManagerPut::ActionPmidManagerPut(const typename Data::Name &data_name,
                                           const uint32_t size)
    : kDataName(GetDataNameVariant(data_name.type, data_name.raw_name)),
      kSize(size) {}

ActionPmidManagerPut::ActionPmidManagerPut(const std::string& serialised_action)
  : kDataName([&serialised_action]()->DataNameVariant {
            protobuf::ActionPmidManagerPut action_put_proto;
            action_put_proto.ParseFromString(serialised_action);
              return GetDataNameVariant(static_cast<DataTagValue>(action_put_proto.data_type()),
                                        Identity(action_put_proto.data_name()));
          }()),
    kSize([&serialised_action]()->uint32_t {
            protobuf::ActionPmidManagerPut action_put_proto;
            action_put_proto.ParseFromString(serialised_action);
              return action_put_proto.size();
          }()) {}

ActionPmidManagerPut::ActionPmidManagerPut(const ActionPmidManagerPut& other)
  : kDataName(other.kDataName),
    kSize(other.kSize) {}

ActionPmidManagerPut::ActionPmidManagerPut(ActionPmidManagerPut&& other)
  : kDataName(other.kDataName),
    kSize(std::move(other.kSize)) {}

std::string ActionPmidManagerPut::Serialise() const {
  protobuf::ActionPmidManagerPut action_put_proto;
  action_put_proto.set_data_type(
    static_cast<int32_t>(boost::apply_visitor(GetTagValueVisitor(), kDataName)));
  action_put_proto.set_data_name(
    boost::apply_visitor(GetIdentityVisitor(), kDataName).string());
  action_put_proto.set_size(kSize);
  return action_put_proto.SerializeAsString();
}

void ActionPmidManagerPut::operator()(boost::optional<PmidManagerValue>& value) const {
  if (!value)
    value.reset(PmidManagerValue());
  value->Add(kDataName, kSize);
}

bool operator==(const ActionPmidManagerPut& lhs, const ActionPmidManagerPut& rhs) {
  return lhs.kDataName == rhs.kDataName &&
         lhs.kSize == rhs.kSize;
}

bool operator!=(const ActionPmidManagerPut& lhs, const ActionPmidManagerPut& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault
}  // namespace maidsafe
