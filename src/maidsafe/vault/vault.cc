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

namespace maidsafe {

namespace vault {

routing::HandleGetReturn VaultFacade::HandleGet(routing::SourceAddress from,
    routing::Authority /* from_authority */, routing::Authority authority, DataTagValue data_type,
        Identity data_name) {
  switch (authority) {
    case routing::Authority::client_manager:
      if (data_type == DataTagValue::kImmutableDataValue)
        MaidManager::template HandleGet<ImmutableData>(from, data_name);
      else if (data_type == DataTagValue::kMutableDataValue)
        return MaidManager::template HandleGet<MutableData>(from, data_name);
      break;
    case routing::Authority::nae_manager:
      if (data_type == DataTagValue::kImmutableDataValue)
        return DataManager::template HandleGet<ImmutableData>(from, data_name);
      else if (data_type == DataTagValue::kMutableDataValue)
        return DataManager::template HandleGet<MutableData>(from, data_name);
      break;
    case routing::Authority::node_manager:
      // Get doesn't go through PmidManager anymore
//      if (data_type == DataTagValue::kImmutableDataValue)
//        return PmidManager::template HandleGet<ImmutableData>(from, data_name);
//      else if (data_type == DataTagValue::kMutableDataValue)
//        PmidManager::template HandleGet<MutableData>(from, data_name);
      break;
    case routing::Authority::managed_node:
      if (data_type == DataTagValue::kImmutableDataValue)
        return PmidNode::template HandleGet<ImmutableData>(from, data_name);
      else if (data_type == DataTagValue::kMutableDataValue)
        return PmidNode::template HandleGet<MutableData>(from, data_name);
      break;
    default:
      break;
  }
  return boost::make_unexpected(MakeError(VaultErrors::failed_to_handle_request));
}

routing::HandlePutPostReturn VaultFacade::HandlePut(routing::SourceAddress from,
    routing::DestinationAddress dest, routing::Authority from_authority,
        routing::Authority to_authority, DataTagValue data_type, SerialisedData serialised_data) {
  switch (to_authority) {
    case routing::Authority::client_manager:
      if (from_authority != routing::Authority::client)
        break;
      if (data_type == DataTagValue::kImmutableDataValue)
        return MaidManager::HandlePut(from, ParseData<ImmutableData>(serialised_data));
      else if (data_type == DataTagValue::kMutableDataValue)
        return MaidManager::HandlePut(from, ParseData<MutableData>(serialised_data));
      else if (data_type == DataTagValue::kPmidValue)
        return MaidManager::HandlePut(from, ParseData<passport::PublicPmid>(serialised_data));
    case routing::Authority::nae_manager:
      if (from_authority != routing::Authority::client_manager)
        break;
      if (data_type == DataTagValue::kImmutableDataValue)
        return DataManager::HandlePut(from, ParseData<ImmutableData>(serialised_data));
      else if (data_type == DataTagValue::kMutableDataValue)
        return DataManager::HandlePut(from, ParseData<MutableData>(serialised_data));
      break;
    case routing::Authority::node_manager:
      if (data_type == DataTagValue::kImmutableDataValue)
        return PmidManager::HandlePut(dest, ParseData<ImmutableData>(serialised_data));
      else if (data_type == DataTagValue::kMutableDataValue)
        return PmidManager::template HandlePut<MutableData>(
                   dest, ParseData<MutableData>(serialised_data));
      break;
    case routing::Authority::managed_node:
      if (data_type == DataTagValue::kImmutableDataValue)
        return PmidNode::HandlePut(from, ParseData<ImmutableData>(serialised_data));
      else if (data_type == DataTagValue::kMutableDataValue)
        return PmidNode::HandlePut(from, ParseData<MutableData>(serialised_data));
      break;
    default:
      break;
  }
  return boost::make_unexpected(MakeError(VaultErrors::failed_to_handle_request));
}

routing::HandlePutPostReturn VaultFacade::HandlePutResponse(routing::SourceAddress from,
    routing::DestinationAddress dest, routing::Authority from_authority,
        routing::Authority to_authority, maidsafe_error return_code,
            DataTagValue data_type, SerialisedData serialised_data) {
  switch (to_authority) {
    case routing::Authority::nae_manager:
      if (from_authority != routing::Authority::node_manager)
        break;
      if (data_type == DataTagValue::kImmutableDataValue)
        return DataManager::template HandlePutResponse<ImmutableData>(
            ParseData<ImmutableData>(serialised_data).name(), dest, return_code);
      else if (data_type == DataTagValue::kMutableDataValue)
        return DataManager::template HandlePutResponse<MutableData>(
            ParseData<MutableData>(serialised_data).name(), dest, return_code);
      break;
    case routing::Authority::node_manager:
      if (from_authority != routing::Authority::managed_node)
        break;
      if (data_type == DataTagValue::kImmutableDataValue)
        return PmidManager::HandlePutResponse(from, return_code,
                                              ParseData<ImmutableData>(serialised_data));
      else if (data_type == DataTagValue::kMutableDataValue)
        return PmidManager::HandlePutResponse(from, return_code,
                                              ParseData<MutableData>(serialised_data));
      break;
    default:
      break;
  }
  return boost::make_unexpected(MakeError(VaultErrors::failed_to_handle_request));
}

}  // namespace vault

}  // namespace maidsafe

