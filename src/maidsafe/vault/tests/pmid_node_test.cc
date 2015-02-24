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

#include "boost/filesystem.hpp"

#include "maidsafe/common/test.h"
#include "maidsafe/common/data_types/immutable_data.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/passport/types.h"

#include "maidsafe/routing/types.h"
#include "maidsafe/routing/source_address.h"

#include "maidsafe/vault/vault.h"

namespace maidsafe {

namespace vault {

namespace test {

class PmidNodeTest : public testing::Test {
 public:
  PmidNodeTest() = default;

 protected:
  PmidNode<VaultFacade> pmid_node_ {
      *maidsafe::test::CreateTestPath("MaidSafe_Vault_PmidNode")}

};

}

} // namespace vault

} // namespace maidsafe
