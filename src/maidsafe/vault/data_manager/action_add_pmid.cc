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

#include "maidsafe/vault/data_manager/action_add_pmid.h"

#include "maidsafe/common/error.h"

#include "maidsafe/data_types/data_name_variant.h"

#include "maidsafe/vault/data_manager/action_add_pmid.pb.h"
#include "maidsafe/vault/data_manager/value.h"

namespace maidsafe {

namespace vault {

ActionDataManagerAddPmid::ActionDataManagerAddPmid(const PmidName& pmid_name, int32_t size)
    : kPmidName(pmid_name), kSize(size) {}

ActionDataManagerAddPmid::ActionDataManagerAddPmid(const std::string& serialised_action)
    : kPmidName([&serialised_action]()->PmidName {
        protobuf::ActionDataManagerAddPmid action_add_pmid_proto;
        if (!action_add_pmid_proto.ParseFromString(serialised_action))
          BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
        return PmidName(Identity(action_add_pmid_proto.pmid_name()));
      }()),
      kSize([&serialised_action]()->int32_t {
        protobuf::ActionDataManagerAddPmid action_add_pmid_proto;
        if (!action_add_pmid_proto.ParseFromString(serialised_action))
          BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
        return action_add_pmid_proto.size();
      }()) {}

ActionDataManagerAddPmid::ActionDataManagerAddPmid(const ActionDataManagerAddPmid& other)
    : kPmidName(other.kPmidName), kSize(other.kSize) {}

ActionDataManagerAddPmid::ActionDataManagerAddPmid(ActionDataManagerAddPmid&& other)
    : kPmidName(std::move(other.kPmidName)), kSize(std::move(other.kSize)) {}

std::string ActionDataManagerAddPmid::Serialise() const {
  protobuf::ActionDataManagerAddPmid action_add_pmid_proto;
  action_add_pmid_proto.set_pmid_name(kPmidName->string());
  action_add_pmid_proto.set_size(kSize);
  return action_add_pmid_proto.SerializeAsString();
}

detail::DbAction ActionDataManagerAddPmid::operator()(std::unique_ptr<DataManagerValue>& value) {
  if (!value)
    value.reset(new DataManagerValue(kPmidName, kSize));
  else
    value->AddPmid(kPmidName);

  if (value->Subscribers() == 0)
    value->IncrementSubscribers();
  return detail::DbAction::kPut;
}

bool operator==(const ActionDataManagerAddPmid& lhs, const ActionDataManagerAddPmid& rhs) {
  return lhs.kPmidName == rhs.kPmidName &&
         lhs.kSize == rhs.kSize;
}

bool operator!=(const ActionDataManagerAddPmid& lhs, const ActionDataManagerAddPmid& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
