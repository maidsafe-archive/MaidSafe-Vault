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

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_SERVICE_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_SERVICE_H_

#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <vector>

#include "maidsafe/common/types.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/message.h"
#include "maidsafe/nfs/public_key_getter.h"

#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/db.h"
#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

struct PmidRegistrationOp;
struct GetPmidTotalsOp;
class MaidManagerMetadata;

class MaidManagerService {
 public:
  MaidManagerService(const passport::Pmid& pmid,
                     routing::Routing& routing,
                     nfs::PublicKeyGetter& public_key_getter,
                     Db& db);
  // Handling of received requests (sending of requests is done via nfs_ object).
  template<typename Data>
  void HandleMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  void HandleMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  void HandleChurnEvent(std::shared_ptr<routing::MatrixChange> matrix_change);
  static int DefaultPaymentFactor() { return kDefaultPaymentFactor_; }

 private:
  MaidManagerService(const MaidManagerService&);
  MaidManagerService& operator=(const MaidManagerService&);
  MaidManagerService(MaidManagerService&&);
  MaidManagerService& operator=(MaidManagerService&&);

  void ValidateDataSender(const nfs::Message& message) const;
  void ValidateGenericSender(const nfs::Message& message) const;

  // =============== Put/Delete data ===============================================================
  template<typename Data>
  void HandlePut(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void HandleDelete(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  typedef std::true_type UniqueDataType;
  template<typename Data>
  void SendEarlySuccessReply(const nfs::Message& /*message*/,
                             const routing::ReplyFunctor& /*reply_functor*/,
                             bool /*low_space*/,
                             UniqueDataType) {}
  typedef std::false_type NonUniqueDataType;
  template<typename Data>
  void SendEarlySuccessReply(const nfs::Message& message,
                             const routing::ReplyFunctor& reply_functor,
                             bool low_space,
                             NonUniqueDataType);
  void SendReplyAndAddToAccumulator(const nfs::Message& message,
                                    const routing::ReplyFunctor& reply_functor,
                                    const nfs::Reply& reply);

  template<typename Data>
  void HandlePutResult(const nfs::Reply& overall_result,
                       const nfs::Message& message,
                       routing::ReplyFunctor client_reply_functor,
                       bool low_space,
                       UniqueDataType);

  template<typename Data>
  void HandlePutResult(const nfs::Reply& overall_result,
                       const nfs::Message& message,
                       routing::ReplyFunctor client_reply_functor,
                       bool low_space,
                       NonUniqueDataType);

  template<typename Data, nfs::MessageAction action>
  void AddLocalUnresolvedEntryThenSync(const nfs::Message& message, int32_t cost);

  // =============== Pmid registration =============================================================
  void HandlePmidRegistration(const nfs::Message& message,
                              const routing::ReplyFunctor& reply_functor);
  template<typename PublicFobType>
  void ValidatePmidRegistration(const nfs::Reply& reply,
                                typename PublicFobType::name_type public_fob_name,
                                std::shared_ptr<PmidRegistrationOp> pmid_registration_op);
  void FinalisePmidRegistration(std::shared_ptr<PmidRegistrationOp> pmid_registration_op);

  // =============== Sync ==========================================================================
  void Sync(const MaidName& account_name);
  void HandleSync(const nfs::Message& message);

  // =============== Account transfer ==============================================================
  void TransferAccount(const MaidName& account_name, const NodeId& new_node);
  void HandleAccountTransfer(const nfs::Message& message);

  // =============== PMID totals ===================================================================
  void UpdatePmidTotals(const MaidName& account_name);
  void UpdatePmidTotalsCallback(const std::string& serialised_reply,
                                std::shared_ptr<GetPmidTotalsOp> op_data);

  routing::Routing& routing_;
  nfs::PublicKeyGetter& public_key_getter_;
  std::mutex accumulator_mutex_, metadata_mutex_;
  Accumulator<MaidName> accumulator_;
  std::vector<MaidManagerMetadata> accounts_;
  MaidManagerNfs nfs_;
  static const int kPutRepliesSuccessesRequired_;
  static const int kDefaultPaymentFactor_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/maid_manager/service-inl.h"

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_SERVICE_H_
