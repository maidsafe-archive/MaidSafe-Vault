/*  Copyright 2012 MaidSafe.net limited

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

#ifndef MAIDSAFE_VAULT_MPID_MANAGER_MPID_MANAGER_HOLDER_SERVICE_H_
#define MAIDSAFE_VAULT_MPID_MANAGER_MPID_MANAGER_HOLDER_SERVICE_H_

#include <vector>

#include "maidsafe/common/rsa.h"
#include "maidsafe/routing/routing_api.h"

// #include "maidsafe/vault/disk_based_storage.h"

namespace maidsafe {

namespace vault {

class MpidAccountHolder {
 public:
  MpidAccountHolder(routing::Routing& routing, const boost::filesystem::path& vault_root_dir);
  ~MpidAccountHolder();
  //  template <typename Data>
  //  void HandleMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
      //  void HandleMessage(const nfs::Message& /*message*/, const routing::ReplyFunctor&
      // /*reply_functor*/) {}
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

// template<typename Data>
// void MpidAccountHolder::HandleMessage(const nfs::Message& /*message*/,
//                                      const routing::ReplyFunctor& /*reply_functor*/) {}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MPID_MANAGER_MPID_MANAGER_HOLDER_SERVICE_H_
