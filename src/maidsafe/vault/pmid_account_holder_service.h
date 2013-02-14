/***************************************************************************************************
 *  Copyright 2012 PmidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of PmidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of PmidSafe.net.                                  *
 **************************************************************************************************/

#ifndef MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_SERVICE_H_
#define MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_SERVICE_H_

#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/common/rsa.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/routing/routing_api.h"

#include "maidsafe/nfs/accumulator.h"
#include "maidsafe/nfs/data_message.h"
#include "maidsafe/nfs/generic_message.h"
#include "maidsafe/nfs/public_key_getter.h"
#include "maidsafe/nfs/response_mapper.h"

#include "maidsafe/vault/pmid_account_handler.h"
#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

class PmidAccountHolderService {
 public:
  PmidAccountHolderService(nfs::NfsResponseMapper& response_mapper,
                           routing::Routing& routing,
                           nfs::PublicKeyGetter& public_key_getter,
                           const boost::filesystem::path& vault_root_dir);
  template<typename Data>
  void HandleDataMessage(const nfs::DataMessage& data_message,
                         const routing::ReplyFunctor& reply_functor);
  void HandleGenericMessage(const nfs::GenericMessage& generic_message,
                            const routing::ReplyFunctor& reply_functor);
  void TriggerSync();

 private:
  template<typename Data>
  void HandlePut(const nfs::DataMessage& data_message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void HandleDelete(const nfs::DataMessage& data_message,
                    const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void ValidateDataMessage(const nfs::DataMessage& data_message) const;
  template<typename Data>
  void AdjustAccount(const nfs::DataMessage& data_message);
  template<typename Data>
  void SendDataMessage(const nfs::DataMessage& data_message);

  bool HandleReceivedSyncData(const NonEmptyString& serialised_account);

  void CheckAccounts();
  bool AssessRange(const PmidName& account_name,
                   PmidAccount::DataHolderStatus account_status,
                   bool is_connected);
  void ValidateDataMessage(const nfs::DataMessage& data_message) const;
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
                    const std::set<PmidName>& metadata_manager_ids,
                    bool node_up);

  routing::Routing& routing_;
  nfs::PublicKeyGetter& public_key_getter_;
  nfs::Accumulator<PmidName> accumulator_;
  PmidAccountHandler pmid_account_handler_;
  PmidAccountHolderNfs nfs_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/pmid_account_holder_service-inl.h"

#endif  // MAIDSAFE_VAULT_PMID_ACCOUNT_HOLDER_SERVICE_H_
