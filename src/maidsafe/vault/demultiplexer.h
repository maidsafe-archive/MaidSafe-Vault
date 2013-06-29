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

#ifndef MAIDSAFE_VAULT_DEMULTIPLEXER_H_
#define MAIDSAFE_VAULT_DEMULTIPLEXER_H_

#include <string>

#include "maidsafe/common/types.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/message.h"


namespace maidsafe {

namespace vault {

class MaidAccountHolderService;
class VersionManagerService;
class DataManagerService;
class PmidAccountHolderService;
class DataHolderService;

class Demultiplexer {
 public:
  Demultiplexer(MaidAccountHolderService& maid_manager_service,
                VersionManagerService& version_manager_service,
                DataManagerService& metadata_manager_service,
                PmidAccountHolderService& pmid_manager_service,
                DataHolderService& pmid_node);
  void HandleMessage(const std::string& serialised_message,
                     const routing::ReplyFunctor& reply_functor);
  bool GetFromCache(std::string& serialised_message);
  void StoreInCache(const std::string& serialised_message);

 private:
  template<typename MessageType>
  void PersonaHandleMessage(const MessageType& message, const routing::ReplyFunctor& reply_functor);
  NonEmptyString HandleGetFromCache(const nfs::Message& message);
  void HandleStoreInCache(const nfs::Message& message);

  MaidAccountHolderService& maid_manager_service_;
  VersionManagerService& version_manager_service_;
  DataManagerService& metadata_manager_service_;
  PmidAccountHolderService& pmid_manager_service_;
  DataHolderService& pmid_node_;
};

template<>
void Demultiplexer::PersonaHandleMessage<nfs::Message>(
    const nfs::Message& message,
    const routing::ReplyFunctor& reply_functor);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DEMULTIPLEXER_H_
