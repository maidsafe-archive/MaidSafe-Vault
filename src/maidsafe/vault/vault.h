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

#ifndef MAIDSAFE_VAULT_VAULT_H_
#define MAIDSAFE_VAULT_VAULT_H_

#include "boost/expected/expected.hpp"

#include "maidsafe/common/data_types/immutable_data.h"
#include "maidsafe/common/data_types/mutable_data.h"

#include "maidsafe/vault/tests/fake_routing.h"  // FIXME(Prakash) replace fake routing with real routing
#include "maidsafe/vault/data_manager.h"
#include "maidsafe/vault/maid_manager.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.h"
#include "maidsafe/vault/pmid_node.h"


namespace maidsafe {

namespace vault {


class VaultFacade : public MaidManager<VaultFacade>,
                    public DataManager<VaultFacade>,
                    public PmidManager<VaultFacade>,
                    public PmidNode<VaultFacade>,
                    public routing::test::FakeRouting<VaultFacade> {
 public:
  VaultFacade()
    : MaidManager<VaultFacade>(),
      DataManager<VaultFacade>(),
      PmidManager<VaultFacade>(),
      PmidNode<VaultFacade>(),
      routing::test::FakeRouting<VaultFacade>() {
  }

  ~VaultFacade() = default;

  enum class FunctorType { FunctionOne, FunctionTwo };
  enum class DataTypeEnum { ImmutableData, MutableData, End };

  template <typename DataType>
  routing::HandleGetReturn HandleGet(routing::SourceAddress from, routing::Authority from_authority,
                                     routing::Authority authority, DataType data_type,
                                     Identity data_name);

  template <typename DataType>
  routing::HandlePutPostReturn HandlePut(routing::SourceAddress from,
      routing::Authority from_authority, routing::Authority authority, DataType data_type);

  bool HandlePost(const routing::SerialisedMessage& message);
  // not in local cache do upper layers have it (called when we are in target group)
   template <typename DataType>
  boost::expected<routing::SerialisedMessage, maidsafe_error> HandleGet(routing::Address) {
    return boost::make_unexpected(MakeError(CommonErrors::no_such_element));
  }
  // default put is allowed unless prevented by upper layers
  bool HandlePut(routing::Address, routing::SerialisedMessage);
  // if the implementation allows any put of data in unauthenticated mode
  bool HandleUnauthenticatedPut(routing::Address, routing::SerialisedMessage);
  void HandleChurn(routing::CloseGroupDifference diff);
};

template <typename DataType>
routing::HandleGetReturn VaultFacade::HandleGet(routing::SourceAddress from,
    routing::Authority /* from_authority */, routing::Authority authority, DataType data_type,
        Identity data_name) {
  switch (authority) {
    case routing::Authority::client_manager:
      if (data_type == DataTypeEnum::ImmutableData)
        return MaidManager::template HandleGet<ImmutableData>(from, data_name);
      else if (data_type == DataTypeEnum::MutableData)
        return MaidManager::template HandleGet<ImmutableData>(from, data_name);
      break;
    case routing::Authority::nae_manager:
      if (data_type == DataTypeEnum::ImmutableData)
        return DataManager::template HandleGet<ImmutableData>(from, data_name);
      else if (data_type == DataTypeEnum::MutableData)
        return DataManager::template HandleGet<ImmutableData>(from, data_name);
      break;
    case routing::Authority::node_manager:
      if (data_type == DataTypeEnum::ImmutableData)
        return PmidManager::template HandleGet<ImmutableData>(from, data_name);
      else if (data_type == DataTypeEnum::MutableData)
        PmidManager::template HandleGet<ImmutableData>(from, data_name);
      break;
    case routing::Authority::managed_node:
      if (data_type == DataTypeEnum::ImmutableData)
        return PmidNode::template HandleGet<ImmutableData>(from, data_name);
      else if (data_type == DataTypeEnum::MutableData)
        return PmidNode::template HandleGet<ImmutableData>(from, data_name);
      break;
    default:
      break;
  }
}

template <typename DataType>
routing::HandlePutPostReturn VaultFacade::HandlePut(routing::SourceAddress from,
    routing::Authority from_authority, routing::Authority authority, DataType data_type) {
  switch (authority) {
    case routing::Authority::client_manager:
      if (from_authority != routing::Authority::client)
        break;
      if (data_type == DataTypeEnum::ImmutableData)
        return MaidManager::template HandlePut<ImmutableData>(from, data_type);
      else if (data_type == DataTypeEnum::MutableData)
        return MaidManager::template HandlePut<MutableData>(from, data_type);
    case routing::Authority::nae_manager:
      if (from_authority != routing::Authority::client_manager)
        break;
      if (data_type == DataTypeEnum::ImmutableData)
        return DataManager::template HandlePut<ImmutableData>(from, data_type);
      else if (data_type == DataTypeEnum::MutableData)
        return DataManager::template HandlePut<MutableData>(from, data_type);
      break;
    case routing::Authority::node_manager:
      if (data_type == DataTypeEnum::ImmutableData)
        return PmidManager::template HandlePut<ImmutableData>(from, data_type);
      else if (data_type == DataTypeEnum::MutableData)
        return PmidManager::template HandlePut<MutableData>(from, data_type);
      break;
    case routing::Authority::managed_node:
      if (data_type == DataTypeEnum::ImmutableData)
        return PmidNode::template HandlePut<ImmutableData>(from, data_type);
      else if (data_type == DataTypeEnum::MutableData)
        return PmidNode::template HandlePut<MutableData>(from, data_type);
      break;
    default:
      break;
  }
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VAULT_H_
