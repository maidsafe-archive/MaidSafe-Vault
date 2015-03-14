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

#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

routing::HandleGetReturn VaultFacade::HandleGet(routing::SourceAddress from,
                                                routing::Authority /* from_authority */,
                                                routing::Authority authority,
                                                Data::NameAndTypeId name_and_type_id) {
  switch (authority) {
    case routing::Authority::client_manager:
      if (name_and_type_id.type_id == detail::TypeId<ImmutableData>::value)
        MaidManager::template HandleGet<ImmutableData>(from, name_and_type_id.name);
      else if (name_and_type_id.type_id == detail::TypeId<MutableData>::value)
        return MaidManager::template HandleGet<MutableData>(from, name_and_type_id.name);
      break;
    case routing::Authority::nae_manager:
      if (name_and_type_id.type_id == detail::TypeId<ImmutableData>::value)
        return DataManager::template HandleGet<ImmutableData>(from, name_and_type_id.name);
      else if (name_and_type_id.type_id == detail::TypeId<MutableData>::value)
        return DataManager::template HandleGet<MutableData>(from, name_and_type_id.name);
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
        return PmidManager::template HandlePut<MutableData>(
                   from, Parse<MutableData>(serialised_data));
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
  MutableData sdv_wrapper(Parse<MutableData>(message));
  StructuredDataVersions sdv(20, 1);
  sdv.ApplySerialised(StructuredDataVersions::serialised_type(sdv_wrapper.Value()));
  return VersionHandler::HandlePost(sdv);
}

}  // namespace vault

}  // namespace maidsafe

