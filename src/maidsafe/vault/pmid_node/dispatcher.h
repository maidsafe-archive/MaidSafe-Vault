/* Copyright 2013 MaidSafe.net limited

This MaidSafe Software is licensed under the MaidSafe.net Commercial License, version 1.0 or later,
and The General Public License (GPL), version 3. By contributing code to this project You agree to
the terms laid out in the MaidSafe Contributor Agreement, version 1.0, found in the root directory
of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available at:

http://www.novinet.com/license

Unless required by applicable law or agreed to in writing, software distributed under the License is
distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.
*/

#ifndef MAIDSAFE_VAULT_PMID_NODE_DISPATCHER_H_
#define MAIDSAFE_VAULT_PMID_NODE_DISPATCHER_H_

#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/message_types.h"

#include "maidsafe/vault/types.h"

namespace maidsafe {

namespace vault {

class PmidNodeDispatcher {
 public:
  PmidNodeDispatcher(routing::Routing& routing);

  void SendGetRequest(const nfs::DataName& data_name);
  void SendAccountTransferRequest();

 private:
  PmidNodeDispatcher();
  PmidNodeDispatcher(const PmidNodeDispatcher&);
  PmidNodeDispatcher(PmidNodeDispatcher&&);
  PmidNodeDispatcher& operator=(PmidNodeDispatcher);

  routing::GroupSource Sender(const MaidName& account_name) const;

  routing::Routing& routing_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PMID_NODE_DISPATCHER_H_
