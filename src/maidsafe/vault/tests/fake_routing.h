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

#ifndef MAIDSAFE_VAULT_TEST_FAKE_ROUTING_H_
#define MAIDSAFE_VAULT_TEST_FAKE_ROUTING_H_

#include "maidsafe/routing/types.h"
#include "maidsafe/routing/source_address.h"

namespace maidsafe {

namespace routing {

namespace test {

template <typename Child>
class FakeRouting {
 public:
  FakeRouting() {}

  FakeRouting(const FakeRouting&) = delete;
  FakeRouting(FakeRouting&&) = delete;
  FakeRouting& operator=(const FakeRouting&) = delete;
  FakeRouting& operator=(FakeRouting&&) = delete;
  ~FakeRouting() = default;

  template <typename DataType>
  routing::HandlePutPostReturn TriggerHandleGet(SourceAddress from, Authority from_authority,
      Authority authority, DataType data_type, Identity data_name) {
    return static_cast<Child*>(this)->HandleGet(from, from_authority, authority, data_type,
                                                data_name);
  }
};

}  // namespace test
}  // namespace routing
} // namespace maidsafe
#endif  // MAIDSAFE_VAULT_TEST_FAKE_ROUTING_H_
