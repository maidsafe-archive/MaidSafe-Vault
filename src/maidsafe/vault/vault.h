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

#include "maidsafe/vault/tests/fake_routing.h"
#include "maidsafe/vault/data_manager.h"
#include "maidsafe/vault/maid_manager.h"
#include "maidsafe/vault/pmid_manager.h"
#include "maidsafe/vault/pmid_node.h"




namespace maidsafe {

namespace vault {

class VaultFacade;


class VaultFacade : public MaidManager<VaultFacade>,
                    public DataManager<VaultFacade>,
                    public PmidManager<VaultFacade>,
                    public PmidNode<VaultFacade>,
                    public routing::test::FakeRouting<VaultFacade> {
 public:
  VaultFacade(asio::io_service& io_service, boost::filesystem::path db_location,
              const passport::Pmid& pmid)
    : DataManager<VaultFacade>(),
      routing::test::FakeRouting<VaultFacade>(io_service, db_location, pmid) {
  }

  ~VaultFacade() = default;

  enum class FunctorType { FunctionOne, FunctionTwo };
  enum class DataTypeEnum { ImmutableData, MutableData, End };

  template <typename DataType>
  void HandleGet(routing::SourceAddress from, routing::Authority /* from_authority */,
                 routing::Authority authority, DataType data_type, Identity data_name);

  template <typename DataType>
  void HandlePut(routing::SourceAddress from, routing::Authority from_authority,
                 routing::Authority authority, DataType data_type);

  bool HandlePost(const routing::SerialisedMessage&) { return false; }
   template <typename DataType>
  boost::expected<routing::SerialisedMessage, maidsafe_error> HandleGet(routing::Address) {
    return boost::make_unexpected(MakeError(CommonErrors::no_such_element));
  }
  // default put is allowed unless prevented by upper layers
  bool HandlePut(routing::Address, routing::SerialisedMessage) { return true; }
  // if the implementation allows any put of data in unauthenticated mode
  bool HandleUnauthenticatedPut(routing::Address, routing::SerialisedMessage) { return true; }
  void HandleChurn(routing::CloseGroupDifference /*diff*/) {
//    MaidManager::HandleChurn(diff);
//    DataManager::HandleChurn(diff);
//    PmidManager::HandleChurn(diff);
  }
};

template <typename DataType>
void VaultFacade::HandleGet(routing::SourceAddress from, routing::Authority /* from_authority */,
               routing::Authority authority, DataType data_type, Identity data_name) {
  switch (authority) {
    case routing::Authority::nae_manager:
      if (data_type == DataTypeEnum::ImmutableData)
        DataManager::template HandleGet<ImmutableData>(from, data_name);
      else if (data_type == DataTypeEnum::MutableData)
        DataManager::template HandleGet<ImmutableData>(from, data_name);
      break;
    case routing::Authority::node_manager:
      if (data_type == DataTypeEnum::ImmutableData)
        PmidManager::template HandleGet<ImmutableData>(from, data_name);
      else if (data_type == DataTypeEnum::MutableData)
        PmidManager::template HandleGet<ImmutableData>(from, data_name);
      break;
    case routing::Authority::managed_node:
      if (data_type == DataTypeEnum::ImmutableData)
        PmidNode::template HandleGet<ImmutableData>(from, data_name);
      else if (data_type == DataTypeEnum::MutableData)
        PmidNode::template HandleGet<ImmutableData>(from, data_name);
      break;
    default:
      break;
  }
}

template <typename DataType>
void VaultFacade::HandlePut(routing::SourceAddress from, routing::Authority from_authority,
                            routing::Authority authority, DataType data_type) {
  switch (authority) {
    case routing::Authority::nae_manager:
      if (from_authority != routing::Authority::client_manager)
        break;
      if (data_type == DataTypeEnum::ImmutableData)
        DataManager::template HandlePut<ImmutableData>(from, data_type);
      else if (data_type == DataTypeEnum::MutableData)
        DataManager::template HandlePut<MutableData>(from, data_type);
      break;
    case routing::Authority::node_manager:
      if (data_type == DataTypeEnum::ImmutableData)
        PmidManager::template HandlePut<ImmutableData>(from, data_type);
      else if (data_type == DataTypeEnum::MutableData)
        PmidManager::template HandlePut<MutableData>(from, data_type);
      break;
    case routing::Authority::managed_node:
      if (data_type == DataTypeEnum::ImmutableData)
        PmidNode::template HandlePut<ImmutableData>(from, data_type);
      else if (data_type == DataTypeEnum::MutableData)
        PmidNode::template HandlePut<MutableData>(from, data_type);
      break;
    default:
      break;
  }
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VAULT_H_
