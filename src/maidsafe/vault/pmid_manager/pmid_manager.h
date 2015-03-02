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

#ifndef MAIDSAFE_VAULT_PMID_MANAGER_H_
#define MAIDSAFE_VAULT_PMID_MANAGER_H_

#include "maidsafe/common/types.h"
#include "maidsafe/routing/types.h"

#include "maidsafe/vault/pmid_manager/account.h"

namespace maidsafe {

namespace vault {


template <typename FacadeType>
class PmidManager {
 public:
  PmidManager();
// Get doesn't go through PmidManager anymore
//  template <typename DataType>
//  routing::HandleGetReturn HandleGet(routing::SourceAddress from, Identity data_name);

  template <typename DataType>
  routing::HandlePutPostReturn HandlePut(const routing::DestinationAddress& dest,
                                         const DataType& data);

  template <typename DataType>
  routing::HandlePutPostReturn HandlePutResponse(const routing::SourceAddress& from,
                                                 const maidsafe_error& return_code,
                                                 const DataType& data);

  void HandleChurn(routing::CloseGroupDifference);

 private:
  std::mutex accounts_mutex_;
  std::map<routing::Address, PmidManagerAccount> accounts_;
};

template <typename FacadeType>
PmidManager<FacadeType>::PmidManager()
    : accounts_mutex_(), accounts_() {}

template <typename FacadeType>
template <typename DataType>
routing::HandlePutPostReturn PmidManager<FacadeType>::HandlePut(
    const routing::DestinationAddress& dest, const DataType& data) {
  try {
    std::lock_guard<std::mutex> lock(accounts_mutex_);
    routing::Address pmid_node(dest.first);
    auto itr(accounts_.find(pmid_node));
    if (itr == std::end(accounts_)) {
      // create an empty account for non-registered pmid_node
      auto result(accounts_.insert(std::make_pair(pmid_node, PmidManagerAccount())));
      if (result.second)
        itr = result.first;
      else
        return boost::make_unexpected(MakeError(CommonErrors::unknown));
    }
    itr->second.PutData(data.Serialise()->string().size());
    std::vector<routing::DestinationAddress> dest_addresses;
    dest_addresses.emplace_back(dest);
    return dest_addresses;
  } catch (const maidsafe_error& error) {
    LOG(kWarning) << "PmidManager::HandlePut caught an error during " << error.what();
    return boost::make_unexpected(error);
  }
}


template <typename FacadeType>
template <typename DataType>
routing::HandlePutPostReturn PmidManager<FacadeType>::HandlePutResponse(
    const routing::SourceAddress& from, const maidsafe_error& return_code, const DataType& data) {
  // There shall be no response from pmid_node in case of put success
  assert(return_code.code() != make_error_code(CommonErrors::success));
  std::lock_guard<std::mutex> lock(accounts_mutex_);
  routing::Address pmid_node(from.node_address);
  auto itr(accounts_.find(pmid_node));
  // for PmidManager, the HandlePutResponse shall never return with error,
  // as this may trigger the returned error_code to be sent back to pmid_node
  if (itr != std::end(accounts_)) {
    itr->second.HandleFailure(data.Serialise()->string().size());
  } else {
    LOG(kError) << "PmidManager doesn't hold account for " << HexSubstr(pmid_node.string());
  }
  std::vector<routing::DestinationAddress> dest;
  dest.push_back(std::make_pair(routing::Destination(routing::Address(data.name())),
                                boost::optional<routing::ReplyToAddress>()));
  return routing::HandlePutPostReturn(dest);
}

}  // namespace vault

}  // namespace maidsafe

#endif // MAIDSAFE_VAULT_PMID_MANAGER_H_
