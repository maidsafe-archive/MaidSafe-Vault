/*  Copyright 2015 MaidSafe.net limited

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

#include "maidsafe/vault/mpid_manager/mpid_manager.h"

namespace maidsafe {

namespace vault {

routing::HandlePostReturn MpidManager::HandlePost(routing::SourceAddress /*from*/,
                                                  MpidMessage mpid_message) {
  if (from.group_address == mpid_message.base.sender) {
    // MpidManagers B received a message from MpidManagers A
    if (!handler_.HasAccount(mpid_message.base.receiver))
      return boost::make_unexpected(MakeError(VaultErrors::no_such_account));
    ImmutableData data(NonEmptyString(Serialise(mpid_message)));
    handler_.Put(data, mpid_message.base.receiver);
    std::vector<routing::DestinationAddress> dest_mpid;
    dest_mpid.emplace_back(routing::Destination(mpid_alert.base.receiver),
                           boost::optional<routing::ReplyToAddress>());
    return routing::HandlePostReturn::value_type(std::make_pair(dest_mpid,
                                                                Serialise(mpid_message)));
  } else {
    // MpidManagers A received a message from mpid_node A
    if (!handler_.HasAccount(mpid_message.base.sender))
      return boost::make_unexpected(MakeError(VaultErrors::no_such_account));
    ImmutableData data(NonEmptyString(Serialise(mpid_message)));
    handler_.Put(data, mpid_message.base.sender);
    MpidAlert mpid_alert(mpid_message.base, MessageIdType(data.name().value.string()));
    std::vector<routing::DestinationAddress> dest_mpid;
    dest_mpid.emplace_back(routing::Destination(mpid_alert.base.receiver),
                           boost::optional<routing::ReplyToAddress>());
    return routing::HandlePostReturn::value_type(std::make_pair(dest_mpid, Serialise(mpid_alert)));
  }
}

routing::HandlePostReturn MpidManager::HandlePost(routing::SourceAddress from,
                                                  MpidAlert mpid_alert) {
  if (from.group_address == mpid_alert.base.sender) {
    // MpidManagers B received a notification from MpidManagers A
    if (!handler_.HasAccount(mpid_alert.base.receiver))
      return boost::make_unexpected(MakeError(VaultErrors::no_such_account));
    ImmutableData data(NonEmptyString(Serialise(mpid_alert)));
    handler_.Put(data, mpid_alert.base.receiver);
    // using pull model, return success which will be dropped in routing i.e. flow terminates
    return boost::make_unexpected(MakeError(CommonErrors::success));
  } else if (from.group_address == mpid_alert.base.receiver) {
    // MpidManagers A received a get request from MpidManagers B
    auto query_result(handler_.GetMessage(ImmutableData::Name(mpid_alert.message_id)));
    if (!query_result.valid())
      return boost::make_unexpected(MakeError(CommonErrors::no_such_element));
    handler_.Delete(ImmutableData::Name(mpid_alert.message_id));
    std::vector<routing::DestinationAddress> dest_mpid;
    dest_mpid.emplace_back(routing::Destination(mpid_alert.base.receiver),
                           boost::optional<routing::ReplyToAddress>());
    return routing::HandlePostReturn::value_type(std::make_pair(dest_mpid,
                                                                Serialise(query_result)));
  } else {
    // MpidManagers B received a get request from mpid_node B
    ImmutableData data(NonEmptyString(Serialise(mpid_alert)));
    if (!handler_.Has(data.name()))
      return boost::make_unexpected(MakeError(CommonErrors::no_such_element));
    handler_.Delete(data.name());
    std::vector<routing::DestinationAddress> dest_mpid;
    dest_mpid.emplace_back(routing::Destination(mpid_alert.base.sender),
                           boost::optional<routing::ReplyToAddress>());
    return routing::HandlePostReturn::value_type(std::make_pair(dest_mpid, Serialise(mpid_alert)));
  }
}

}  // namespace vault

}  // namespace maidsafe
