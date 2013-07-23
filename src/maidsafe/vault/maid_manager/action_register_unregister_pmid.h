/* Copyright 2013 MaidSafe.net limited

This MaidSafe Software is licensed under the MaidSafe.net Commercial License, version 1.0 or later,
and The General Public License (GPL), version 3. By contributing code to this project You agree to
the terms laid out in the MaidSafe Contributor Agreement, version 1.0, found in the root directory
of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available at:

http://www.novinet.com/license

Unless required by applicable law or agreed to in writing, software distributed under the License is
distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.
*/

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_ACTION_REGISTER_UNREGISTER_PMID_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_ACTION_REGISTER_UNREGISTER_PMID_H_

#include <string>

#include "boost/optional/optional.hpp"

#include "maidsafe/nfs/pmid_registration.h"
#include "maidsafe/nfs/types.h"


namespace maidsafe {

namespace vault {

class MaidManagerMetadata;

template<bool Unregister>
struct ActionRegisterUnregisterPmid {
  explicit ActionRegisterUnregisterPmid(const nfs::PmidRegistration& pmid_registration);
  explicit ActionRegisterUnregisterPmid(const std::string& serialised_action);
  ActionRegisterUnregisterPmid(const ActionRegisterUnregisterPmid& other);
  ActionRegisterUnregisterPmid(ActionRegisterUnregisterPmid&& other);
  std::string Serialise() const;

  void operator()(boost::optional<MaidManagerMetadata>& value) const;

  static const nfs::MessageAction kActionId;
  const nfs::PmidRegistration kPmidRegistration;

 private:
  ActionRegisterUnregisterPmid();
  ActionRegisterUnregisterPmid& operator=(ActionRegisterUnregisterPmid other);
};

template<>
void ActionRegisterUnregisterPmid<false>::operator()(
    boost::optional<MaidManagerMetadata>& value) const;

template<>
void ActionRegisterUnregisterPmid<true>::operator()(
    boost::optional<MaidManagerMetadata>& value) const;

template<bool Unregister>
bool operator==(const ActionRegisterUnregisterPmid<Unregister>& lhs,
                const ActionRegisterUnregisterPmid<Unregister>& rhs);

template<bool Unregister>
bool operator!=(const ActionRegisterUnregisterPmid<Unregister>& lhs,
                const ActionRegisterUnregisterPmid<Unregister>& rhs);

typedef ActionRegisterUnregisterPmid<false> ActionRegisterPmid;
typedef ActionRegisterUnregisterPmid<true> ActionUnregisterPmid;



// ==================== Implementation =============================================================
template<bool Unregister>
ActionRegisterUnregisterPmid<Unregister>::ActionRegisterUnregisterPmid(
    const nfs::PmidRegistration& pmid_registration_in)
        : kPmidRegistration(pmid_registration_in) {}

template<bool Unregister>
ActionRegisterUnregisterPmid<Unregister>::ActionRegisterUnregisterPmid(
    const std::string& serialised_action)
        : kPmidRegistration([&serialised_action]()->nfs::PmidRegistration::serialised_type {
            protobuf::ActionRegisterUnregisterPmid action_register_pmid_proto;
            if (!action_register_pmid_proto.ParseFromString(serialised_action))
              ThrowError(CommonErrors::parsing_error);
            return nfs::PmidRegistration::serialised_type(NonEmptyString(
                action_register_pmid_proto.serialised_pmid_registration()));
          }()) {
  assert(kPmidRegistration.unregister() == Unregister);
}

template<bool Unregister>
ActionRegisterUnregisterPmid<Unregister>::ActionRegisterUnregisterPmid(
    const ActionRegisterUnregisterPmid& other)
        : kPmidRegistration(other.kPmidRegistration) {}

template<bool Unregister>
ActionRegisterUnregisterPmid<Unregister>::ActionRegisterUnregisterPmid(
    ActionRegisterUnregisterPmid&& other)
        : kPmidRegistration(std::move(other.kPmidRegistration)) {}

template<bool Unregister>
std::string ActionRegisterUnregisterPmid<Unregister>::Serialise() const {
  protobuf::ActionRegisterUnregisterPmid action_register_pmid_proto;
  action_register_pmid_proto.set_serialised_pmid_registration(
      kPmidRegistration.Serialise()->string());
  return action_register_pmid_proto.SerializeAsString();
}

template<bool Unregister>
bool operator==(const ActionRegisterUnregisterPmid<Unregister>& lhs,
                const ActionRegisterUnregisterPmid<Unregister>& rhs) {
  return lhs.kPmidRegistration.maid_name() == rhs.kPmidRegistration.maid_name() &&
         lhs.kPmidRegistration.pmid_name() == rhs.kPmidRegistration.pmid_name() &&
         lhs.kPmidRegistration.unregister() == rhs.kPmidRegistration.unregister();
}

template<bool Unregister>
bool operator!=(const ActionRegisterUnregisterPmid<Unregister>& lhs,
                const ActionRegisterUnregisterPmid<Unregister>& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_ACTION_REGISTER_UNREGISTER_PMID_H_
