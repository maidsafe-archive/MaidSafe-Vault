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

#include "maidsafe/vault/vault.h"

#define COMPANY_NAME DummyValue
#define APPLICATION_NAME DummyValue
#include "maidsafe/common/application_support_directories.h"
#undef COMPANY_NAME
#undef APPLICATION_NAME

#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

boost::filesystem::path VaultDir() {
  static const boost::filesystem::path path(GetHomeDir() / "MaidSafe-Vault");
  return path;
}

routing::HandleGetReturn VaultFacade::HandleGet(routing::SourceAddress from,
                                                routing::Authority /* from_authority */,
                                                routing::Authority authority,
                                                Data::NameAndTypeId name_and_type_id) {
  switch (authority) {
    case routing::Authority::nae_manager:
      if (name_and_type_id.type_id == detail::TypeId<ImmutableData>::value)
        return DataManager::template HandleGet<ImmutableData>(from, name_and_type_id.name);
      else if (name_and_type_id.type_id == detail::TypeId<MutableData>::value)
        return VersionHandler::HandleGet(from, name_and_type_id.name);
      break;
    case routing::Authority::node_manager:
      if (name_and_type_id.type_id == detail::TypeId<ImmutableData>::value)
        return PmidManager::template HandleGet<ImmutableData>(from, name_and_type_id.name);
      else if (name_and_type_id.type_id == detail::TypeId<MutableData>::value)
        PmidManager::template HandleGet<MutableData>(from, name_and_type_id.name);
      break;
    case routing::Authority::managed_node:
      if (name_and_type_id.type_id == detail::TypeId<ImmutableData>::value)
        return PmidNode::template HandleGet<ImmutableData>(from, name_and_type_id.name);
      else if (name_and_type_id.type_id == detail::TypeId<MutableData>::value)
        return PmidNode::template HandleGet<MutableData>(from, name_and_type_id.name);
      break;
    default:
      break;
  }
  return boost::make_unexpected(MakeError(VaultErrors::failed_to_handle_request));
}

routing::HandlePutPostReturn VaultFacade::HandlePut(routing::SourceAddress from,
                                                    routing::Authority from_authority,
                                                    routing::Authority authority,
                                                    DataTypeId data_type_id,
                                                    SerialisedData serialised_data) {
  switch (authority) {
    case routing::Authority::client_manager:
      if (from_authority != routing::Authority::client)
        break;
      if (data_type_id == detail::TypeId<ImmutableData>::value)
        return MaidManager::HandlePut(from, Parse<ImmutableData>(serialised_data));
      else if (data_type_id == detail::TypeId<MutableData>::value)
        return MaidManager::HandlePut(from, Parse<MutableData>(serialised_data));
      else if (data_type_id == detail::TypeId<passport::PublicPmid>::value)
        return MaidManager::HandlePut(from, Parse<passport::PublicPmid>(serialised_data));
    case routing::Authority::nae_manager:
      if (from_authority != routing::Authority::client_manager)
        break;
      if (data_type_id == detail::TypeId<ImmutableData>::value)
        return DataManager::HandlePut(from, Parse<ImmutableData>(serialised_data));
      else if (data_type_id == detail::TypeId<MutableData>::value)
        return DataManager::HandlePut(from, Parse<MutableData>(serialised_data));
      break;
    case routing::Authority::node_manager:
      if (data_type_id == detail::TypeId<ImmutableData>::value)
        return PmidManager::HandlePut(from, Parse<ImmutableData>(serialised_data));
      else if (data_type_id == detail::TypeId<MutableData>::value)
        return PmidManager::template HandlePut<MutableData>(from,
                                                            Parse<MutableData>(serialised_data));
      break;
    case routing::Authority::managed_node:
      if (data_type_id == detail::TypeId<ImmutableData>::value)
        return PmidNode::HandlePut(from, Parse<ImmutableData>(serialised_data));
      else if (data_type_id == detail::TypeId<MutableData>::value)
        return PmidNode::HandlePut(from, Parse<MutableData>(serialised_data));
      break;
    default:
      break;
  }
  return boost::make_unexpected(MakeError(VaultErrors::failed_to_handle_request));
}

bool VaultFacade::HandlePost(const routing::SerialisedMessage& message) {
  return VersionHandler::HandlePost(message);
}

bool VaultFacade::HandlePut(routing::Address /*from*/, routing::SerialisedMessage message) {
  return VersionHandler::HandlePut(message);
}

// MpidManager is ClientManager
routing::HandlePostReturn VaultFacade::HandlePost(routing::SourceAddress from,
    routing::Authority from_authority, routing::Authority authority,
        routing::SerialisedMessage message) {
  switch (authority) {
    case routing::Authority::client_manager:
      if (from_authority == routing::Authority::client) {
        // mpid_node A -> MpidManagers A : post MpidMessage to send message
        // mpid_node B -> MpidManagers B : post MpidAlert to get message
        try {
          MpidMessage mpid_message = Parse<MpidMessage>(message);
          return MpidManager::HandlePost(from, mpid_message);
        } catch (...) {
          MpidAlert mpid_alert = Parse<MpidAlert>(message);
          return MpidManager::HandlePost(from, mpid_alert);
        }
      } else {
        // MpidManagers A -> MpidManagers B : post MpidAlert to notification
        // MpidManagers B -> MpidManagers A : post MpidAlert to get the message
        // MpidManagers A -> MpidManagers B : post MpidMessage
        try {
          MpidMessage mpid_message = Parse<MpidMessage>(message);
          return MpidManager::HandlePost(from, mpid_message);
        } catch (...) {
          MpidAlert mpid_alert = Parse<MpidAlert>(message);
          return MpidManager::HandlePost(from, mpid_alert);
        }
      }
    default:
      break;
  }
  return boost::make_unexpected(MakeError(VaultErrors::failed_to_handle_request));
}

}  // namespace vault

}  // namespace maidsafe
