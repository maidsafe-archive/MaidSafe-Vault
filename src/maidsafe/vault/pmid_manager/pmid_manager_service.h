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

#ifndef MAIDSAFE_VAULT_PMID_MANAGER_PMID_MANAGER_SERVICE_H_
#define MAIDSAFE_VAULT_PMID_MANAGER_PMID_MANAGER_SERVICE_H_

#include <mutex>
#include <set>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/rsa.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/message.h"
#include "maidsafe/nfs/public_key_getter.h"
#include "maidsafe/nfs/reply.h"

#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/pmid_manager/pmid_manager_handler.h"
#include "maidsafe/vault/types.h"


namespace maidsafe {
namespace vault {

class PmidManagerService {
 public:
  PmidManagerService(const passport::Pmid& pmid, routing::Routing& routing, Db& db);
  template<typename Data>
  void HandleMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  void HandleMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  void HandleChurnEvent(routing::MatrixChange matrix_change);

 private:
  PmidManagerService(const PmidManagerService&);
  PmidManagerService& operator=(const PmidManagerService&);
  PmidManagerService(PmidManagerService&&);
  PmidManagerService& operator=(PmidManagerService&&);

  void ValidateDataSender(const nfs::Message& message) const;
  void ValidateGenericSender(const nfs::Message& message) const;

  void SendReplyAndAddToAccumulator(const nfs::Message& message,
                                    const routing::ReplyFunctor& reply_functor,
                                    const nfs::Reply& reply);

  template<typename Data>
  void HandlePut(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void HandleDelete(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);

  void HandleGetPmidTotals(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);

  template<typename Data>
  void AdjustAccount(const nfs::Message& message);
  template<typename Data>
  void SendMessages(const nfs::Message& message);

  template<typename Data>
  void HandlePutResult(const nfs::Reply& overall_result,
                       const nfs::Message& message,
                       routing::ReplyFunctor reply_functor);

  bool HandleReceivedSyncData(const NonEmptyString& serialised_account);


  // =============== Sync ==========================================================================
  void Sync(const PmidName& account_name);
  void HandleSync(const nfs::Message& message);

  // =============== Account transfer ==============================================================
  void TransferAccount(const PmidName& account_name, const NodeId& new_node);
  void HandleAccountTransfer(const nfs::Message& message);

  void CheckAccounts();
  bool AssessRange(const PmidName& account_name,
                   PmidAccount::DataHolderStatus account_status,
                   bool is_connected);
  void ValidateMessage(const nfs::Message& message) const;
  void InformOfDataHolderDown(const PmidName& pmid_name);
  void InformOfDataHolderUp(const PmidName& pmid_name);
  void InformAboutDataHolder(const PmidName& pmid_name, bool node_up);

  bool StatusHasReverted(const PmidName& pmid_name, bool node_up) const;
  void RevertMessages(const PmidName& pmid_name,
                      const std::vector<boost::filesystem::path>::reverse_iterator& begin,
                      std::vector<boost::filesystem::path>::reverse_iterator& current,
                      bool node_up);
  std::set<PmidName> GetDataNamesInFile(const PmidName& pmid_name,
                                        const boost::filesystem::path& path) const;
  void SendMessages(const PmidName& pmid_name,
                    const std::set<PmidName>& data_manager_ids,
                    bool node_up);

  template<typename Data, nfs::MessageAction action>
  void AddLocalUnresolvedEntryThenSync(const nfs::Message& message);
  template<typename Data, nfs::MessageAction action>
  void ReplyToDataManagers(const std::vector<PmidManagerResolvedEntry>& resolved_entries,
                               const PmidName& pmid_name);

  routing::Routing& routing_;
  std::mutex accumulator_mutex_;
  Accumulator<PmidName> accumulator_;
  PmidAccountHandler pmid_account_handler_;
  PmidManagerNfs nfs_;
  static const int kPutRepliesSuccessesRequired_;
  static const int kDeleteRequestsRequired_;
};

}  // namespace vault
}  // namespace maidsafe

#include "maidsafe/vault/pmid_manager/pmid_manager_service-inl.h"

#endif  // MAIDSAFE_VAULT_PMID_MANAGER_PMID_MANAGER_SERVICE_H_
