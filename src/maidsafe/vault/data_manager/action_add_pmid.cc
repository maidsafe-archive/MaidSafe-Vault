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
#include "maidsafe/common/data_types/data_name_variant.h"

#include "maidsafe/vault/data_manager/action_add_pmid.pb.h"
#include "maidsafe/vault/data_manager/value.h"

namespace maidsafe {

namespace vault {

ActionDataManagerAddPmid::ActionDataManagerAddPmid(const PmidName& pmid_name)
    : kPmidName(pmid_name) {}

ActionDataManagerAddPmid::ActionDataManagerAddPmid(const std::string& serialised_action)
    : kPmidName([&serialised_action]()->PmidName {
        protobuf::ActionDataManagerAddPmid action_add_pmid_proto;
        if (!action_add_pmid_proto.ParseFromString(serialised_action))
          BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
        return PmidName(Identity(action_add_pmid_proto.pmid_name()));
      }()) {}

ActionDataManagerAddPmid::ActionDataManagerAddPmid(const ActionDataManagerAddPmid& other)
    : kPmidName(other.kPmidName) {}

ActionDataManagerAddPmid::ActionDataManagerAddPmid(ActionDataManagerAddPmid&& other)
    : kPmidName(std::move(other.kPmidName)) {}

std::string ActionDataManagerAddPmid::Serialise() const {
  protobuf::ActionDataManagerAddPmid action_add_pmid_proto;
  action_add_pmid_proto.set_pmid_name(kPmidName->string());
  return action_add_pmid_proto.SerializeAsString();
}

detail::DbAction ActionDataManagerAddPmid::operator()(std::unique_ptr<DataManagerValue>& value) {
  if (value)
    value->AddPmid(kPmidName);
  return detail::DbAction::kPut;
}

bool operator==(const ActionDataManagerAddPmid& lhs, const ActionDataManagerAddPmid& rhs) {
  return lhs.kPmidName == rhs.kPmidName;
}

bool operator!=(const ActionDataManagerAddPmid& lhs, const ActionDataManagerAddPmid& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
