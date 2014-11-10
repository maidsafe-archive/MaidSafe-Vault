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

#include "maidsafe/vault/pmid_manager/action_delete.h"

#include "maidsafe/common/error.h"

#include "maidsafe/vault/pmid_manager/action_delete.pb.h"

namespace maidsafe {

namespace vault {

ActionPmidManagerDelete::ActionPmidManagerDelete(
  uint64_t size, bool pmid_node_available_in, bool data_failure_in)
    : kSize(size),
      pmid_node_available(pmid_node_available_in),
      data_failure(data_failure_in) {}

ActionPmidManagerDelete::ActionPmidManagerDelete(const std::string& serialised_action)
    : kSize(0), pmid_node_available(true), data_failure(false) {
  protobuf::ActionPmidManagerDelete action_delete_proto;
  if (!action_delete_proto.ParseFromString(serialised_action)) {
    LOG(kError) << "Can't parse ActionPmidManagerDelete from serialised string";
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::parsing_error));
  }
  kSize = action_delete_proto.size();
  pmid_node_available = action_delete_proto.pmid_node_available();
  data_failure = action_delete_proto.data_failure();
}

void ActionPmidManagerDelete::operator()(PmidManagerValue& value) {
  if (pmid_node_available) {
    if (data_failure)
      value.HandleLostData(kSize);
    else
      value.DeleteData(kSize);
  } else {
    value.HandleFailure(kSize);
  }
}

std::string ActionPmidManagerDelete::Serialise() const {
  protobuf::ActionPmidManagerDelete action_delete_proto;
  action_delete_proto.set_size(kSize);
  action_delete_proto.set_pmid_node_available(pmid_node_available);
  action_delete_proto.set_data_failure(data_failure);
  return action_delete_proto.SerializeAsString();
}

bool operator==(const ActionPmidManagerDelete& lhs, const ActionPmidManagerDelete& rhs) {
  return (lhs.kSize ==  rhs.kSize) && (lhs.pmid_node_available ==  rhs.pmid_node_available) &&
         (lhs.data_failure ==  rhs.data_failure);
}

bool operator!=(const ActionPmidManagerDelete& lhs, const ActionPmidManagerDelete& rhs) {
  return !operator==(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
