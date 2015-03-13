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
#include "maidsafe/passport/types.h"

#include "maidsafe/vault/tests/fake_routing.h"  // FIXME(Prakash) replace fake routing with real routing
#include "maidsafe/vault/data_manager/data_manager.h"
#include "maidsafe/vault/maid_manager/maid_manager.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.h"
#include "maidsafe/vault/pmid_node/pmid_node.h"

namespace fs = boost::filesystem;

static fs::path vault_dir{fs::path(getenv("HOME")) / "MaidSafe-Vault"};

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
        DataManager<VaultFacade>(vault_dir),
        PmidManager<VaultFacade>(),
        PmidNode<VaultFacade>(),
        routing::test::FakeRouting<VaultFacade>() {}

  ~VaultFacade() = default;

  enum class FunctorType { FunctionOne, FunctionTwo };

  routing::HandleGetReturn HandleGet(routing::SourceAddress from, routing::Authority from_authority,
                                     routing::Authority authority,
                                     Data::NameAndTypeId name_and_type_id);

  routing::HandlePutPostReturn HandlePut(routing::SourceAddress from,
                                         routing::Authority from_authority,
                                         routing::Authority authority, DataTypeId data_type_id,
                                         SerialisedData serialised_data);

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

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VAULT_H_
