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

#include "maidsafe/vault/pmid_manager/action_set_pmid_health.h"
#include "maidsafe/vault/pmid_manager/action_set_pmid_health.pb.h"

#include "maidsafe/vault/pmid_manager/value.h"

namespace maidsafe {

namespace vault {

ActionPmidManagerSetPmidHealth::ActionPmidManagerSetPmidHealth(int64_t disk_available_size)
    : kDiskAvailableSize(disk_available_size) {}

ActionPmidManagerSetPmidHealth::ActionPmidManagerSetPmidHealth(const std::string& serialised_action)
    : kDiskAvailableSize([&serialised_action]() {
                            protobuf::ActionPmidManagerSetPmidHealth action_disk_size_proto;
                            if (!action_disk_size_proto.ParseFromString(serialised_action))
                              BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
                            return action_disk_size_proto.disk_available_size();
                          }()) {}

ActionPmidManagerSetPmidHealth::ActionPmidManagerSetPmidHealth(
    const ActionPmidManagerSetPmidHealth& other)
        : kDiskAvailableSize(other.kDiskAvailableSize) {}

ActionPmidManagerSetPmidHealth::ActionPmidManagerSetPmidHealth(
    ActionPmidManagerSetPmidHealth&& other)
        : kDiskAvailableSize(std::move(other.kDiskAvailableSize)) {}

std::string ActionPmidManagerSetPmidHealth::Serialise() const {
  protobuf::ActionPmidManagerSetPmidHealth action_disk_size_proto;
  action_disk_size_proto.set_disk_available_size(kDiskAvailableSize);
  return action_disk_size_proto.SerializeAsString();
}

detail::DbAction ActionPmidManagerSetPmidHealth::operator()(
    std::unique_ptr<PmidManagerValue>& metadata) {
  metadata->SetAvailableSize(kDiskAvailableSize);
  return detail::DbAction::kPut;
}

bool operator==(const ActionPmidManagerSetPmidHealth& /*lhs*/,
                const ActionPmidManagerSetPmidHealth& /*rhs*/) {
  return true;
}

bool operator!=(const ActionPmidManagerSetPmidHealth& lhs,
                const ActionPmidManagerSetPmidHealth& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
