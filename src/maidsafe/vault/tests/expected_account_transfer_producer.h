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

#ifndef MAIDSAFE_VAULT_TEST_EXPECTED_ACCOUNT_TRANSFER_PRODUCER_H_
#define MAIDSAFE_VAULT_TEST_EXPECTED_ACCOUNT_TRANSFER_PRODUCER_H_

#include <algorithm>
#include <map>
#include <mutex>
#include <utility>
#include <vector>

#include "boost/optional.hpp"

#include "maidsafe/common/node_id.h"

#include "maidsafe/vault/account_transfer_handler.h"


namespace maidsafe {

namespace vault {

namespace test {

template <typename Persona>
class ExpectedAccountTransferProducer {};

}  // namespace test

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_TEST_EXPECTED_ACCOUNT_TRANSFER_PRODUCER_H_
