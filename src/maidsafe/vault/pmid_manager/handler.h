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

#ifndef MAIDSAFE_VAULT_PMID_MANAGER_HANDLER_H_
#define MAIDSAFE_VAULT_PMID_MANAGER_HANDLER_H_

#include <cstdint>
#include <future>
#include <map>
#include <mutex>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/vault/pmid_manager/pmid_manager.h"
#include "maidsafe/vault/pmid_manager/metadata.h"
#include "maidsafe/vault/types.h"

namespace maidsafe {

namespace vault {
/*
class PmidAccountHandler {
 public:
//  typedef std::map<typename PmidAccount::Name, std::unique_ptr<PmidAccount>> AccountMap;

  explicit PmidAccountHandler(Db& db, const NodeId& this_node_id);

  // Account operations
  void CreateAccount(const PmidName& account_name);
  bool ApplyAccountTransfer(const PmidName& account_name, const NodeId& source_id,
                            const std::string& serialised_pmid_account_details);
  void AddAccount(std::unique_ptr<PmidAccount> pmid_account);
  void DeleteAccount(const PmidName& account_name);
  PmidAccount::PmidNodeStatus PmidNodeStatus(const PmidName& account_name) const;

  void SetPmidNodeDown(const PmidName& account_name);
  void SetPmidNodeUp(const PmidName& account_name);

  void AddLocalUnresolvedEntry(const PmidName& account_name,
                               const PmidManagerUnresolvedEntry& unresolved_entry);
  PmidManagerMetadata GetMetadata(const PmidName& account_name);

  // Sync operations
  std::vector<PmidName> GetAccountNames() const;
  PmidAccount::serialised_type GetSerialisedAccount(const PmidName& account_name,
                                                    bool include_pmid_record) const;
  NonEmptyString GetSyncData(const PmidName& account_name);
  void ApplySyncData(const PmidName& account_name,
                     const NonEmptyString& serialised_unresolved_entries);
  void ReplaceNodeInSyncList(const PmidName& account_name,
                             const NodeId& old_node,
                             const NodeId& new_node);

  // Data operations
  template<typename Data>
  void Put(const PmidName& account_name, const typename Data::Name& data_name, int32_t size);
  template<typename Data>
  void Delete(const PmidName& account_name, const typename Data::Name& data_name);

 private:
  PmidAccountHandler(const PmidAccountHandler&);
  PmidAccountHandler& operator=(const PmidAccountHandler&);
  PmidAccountHandler(PmidAccountHandler&&);
  PmidAccountHandler& operator=(PmidAccountHandler&&);

  const boost::filesystem::path kPmidAccountsRoot_;
  Db& db_;
  const NodeId kThisNodeId_;
  mutable std::mutex mutex_;
  AccountMap pmid_accounts_;
};
*/
}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/pmid_manager/handler-inl.h"

#endif  // MAIDSAFE_VAULT_PMID_MANAGER_HANDLER_H_
