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

#ifndef MAIDSAFE_VAULT_DATA_MANAGER_GET_HELPER_H_
#define MAIDSAFE_VAULT_DATA_MANAGER_GET_HELPER_H_

#include "maidsafe/routing/timer.h"

#include "maidsafe/vault/data_manager/dispatcher.h"

namespace maidsafe {

namespace vault {

class DataManagerGetHelper {
 public:
  DataManagerGetHelper(DatamanagerDispatcher& dispatcher);
  template <typename Data, typename RequestorIdType>
  void SendGetRequest(const typename Data::Name& data_name, const RequestorIdType& requestor,
                      nfs::MessageId message_id);
  template <typename Data>
  void DoGetForReplication(const typename Data::Name& data_name,
                           const std::set<PmidName>& online_pmids);
  template <typename Data>
  void DataManagerGetHelper::GetForReplication(const PmidName& pmid_name,
                                               const typename Data::Name& data_name);
 private:
  routing::Timer<std::pair<PmidName, GetResponseContents>> get_timer_;
};

template <typename Data, typename RequestorIdType>
void DataManagerGetHelper::SendGetRequest(const typename Data::Name& data_name,
                                          const RequestorIdType& requestor,
                                          nfs::MessageId message_id) {
  std::set<PmidName> online_pmids(GetOnlinePmids<Data>(data_name));
  int expected_response_count(static_cast<int>(online_pmids.size()));
  if (expected_response_count == 0) {
    dispatcher_.SendGetResponseFailure(requestor, data_name,
                                       maidsafe_error(CommonErrors::no_such_element), message_id);
    return;
  }
  auto pmid_node_to_get_from(ChoosePmidNodeToGetFrom(online_pmids, data_name));
  std::map<PmidName, IntegrityCheckData> integrity_checks;
  auto get_response_op(
           std::make_shared<detail::GetResponseOp<typename Data::Name, RequestorIdType>>(
               pmid_node_to_get_from, message_id, integrity_checks, data_name, requestor));
  auto functor([=](const std::pair<PmidName, GetResponseContents>& pmid_node_and_contents) {
    {
      std::lock_guard<decltype(close_nodes_change_mutex_)> lock(this->close_nodes_change_mutex_);
      if (stopped_)
        return;
    }
    
    this->DoHandleGetResponse<Data, RequestorIdType>(pmid_node_and_contents.first,
                                                     pmid_node_and_contents.second,
                                                     get_response_op);
  });
  get_timer_.AddTask(detail::Parameters::kDefaultTimeout, functor, 1/*expected_response_count*/,
                     message_id.data);
  dispatcher_.SendGetRequest<Data>(pmid_node_to_get_from, data_name, message_id);
}

template <typename Data>
void DataManagerGetHelper::GetForReplication(const PmidName& pmid_name,
                                             const typename Data::Name& data_name) {
  std::set<PmidName> online_pmids(GetOnlinePmids<Data>(data_name));
  online_pmids.erase(pmid_name);
  DoGetForReplication<Data>(data_name, online_pmids);
}

template <typename Data>
void DataManagerGetHelper::DoGetForReplication(const typename Data::Name& data_name,
                                               const std::set<PmidName>& online_pmids) {
  // Just get, don't do integrity check
  auto functor([=](const std::pair<PmidName, GetResponseContents>& pmid_node_and_contents) {
    this->DoGetResponseForReplication<Data>(pmid_node_and_contents.first, data_name,
                                            pmid_node_and_contents.second);
  });
  nfs::MessageId message_id(get_timer_.NewTaskId());
  get_timer_.AddTask(detail::Parameters::kDefaultTimeout, functor, 1, message_id);
  for (auto& pmid_node : online_pmids) {
    dispatcher_.SendGetRequest<Data>(pmid_node, data_name, message_id);
  }
}
  
}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_GET_HELPER_H_
