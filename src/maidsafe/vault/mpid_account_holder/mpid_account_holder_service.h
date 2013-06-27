/* Copyright 2012 MaidSafe.net limited

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

#ifndef MAIDSAFE_VAULT_MPID_ACCOUNT_HOLDER_MPID_ACCOUNT_HOLDER_SERVICE_H_
#define MAIDSAFE_VAULT_MPID_ACCOUNT_HOLDER_MPID_ACCOUNT_HOLDER_SERVICE_H_

#include <vector>

#include "maidsafe/common/rsa.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/message.h"

// #include "maidsafe/vault/disk_based_storage.h"


namespace maidsafe {

namespace vault {

class MpidAccountHolder {
 public:
  MpidAccountHolder(routing::Routing& routing, const boost::filesystem::path& vault_root_dir);
  ~MpidAccountHolder();
  template<typename Data>
  void HandleMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  void HandleMessage(const nfs::Message& /*message*/, const routing::ReplyFunctor& /*reply_functor*/) {}
  void HandleChurnEvent(routing::MatrixChange matrix_change);

 private:
//  void HandlePutMessage(const Message& message);
//  void HandleGetMessage(const Message& message);
//  void HandlePostMessage(const Message& message);
//  void HandleDeleteMessage(const Message& message);
//  boost::filesystem::path vault_root_dir_;
//  routing::Routing& routing_;
//  DiskBasedStorage disk_storage_;
};

template<typename Data>
void MpidAccountHolder::HandleMessage(const nfs::Message& /*message*/,
                                      const routing::ReplyFunctor& /*reply_functor*/) {}

}  // namespace vault

}  // namespace maidsafe


#endif  // MAIDSAFE_VAULT_MPID_ACCOUNT_HOLDER_MPID_ACCOUNT_HOLDER_SERVICE_H_
