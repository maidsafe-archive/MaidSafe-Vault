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

#include "boost/mpl/vector.hpp"
#include "boost/mpl/insert_range.hpp"
#include "boost/mpl/end.hpp"

#include "maidsafe/common/rsa.h"

#include "maidsafe/routing/routing_api.h"

#include "maidsafe/nfs/client/data_getter.h"

#include "maidsafe/vault/message_types.h"
#include "maidsafe/vault/account_transfer_handler.h"
#include "maidsafe/vault/mpid_manager/mpid_manager.h"
#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/mpid_manager/database.h"
#include "maidsafe/vault/mpid_manager/dispatcher.h"

namespace maidsafe {

namespace vault {

class MpidManagerService {
 public:
  MpidManagerService (const passport::Pmid& pmid, routing::Routing& routing,
                      nfs_client::DataGetter& data_getter,
                      const boost::filesystem::path& vault_root_dir);
  ~MpidManagerService();

  template <typename MessageType>
  void HandleMessage(const MessageType& message, const typename MessageType::Sender& sender,
                     const typename MessageType::Receiver& receiver);

  void HandleChurnEvent(std::shared_ptr<routing::CloseNodesChange> close_nodes_change);

  typedef boost::mpl::vector<> InitialType;
  typedef boost::mpl::insert_range<InitialType,
                                   boost::mpl::end<InitialType>::type,
                                   MpidManagerServiceMessages::types>::type IntermediateType;
//  typedef boost::mpl::insert_range<IntermediateType,
//                                   boost::mpl::end<IntermediateType>::type,
//                                   MpidManagerServiceMessages::types>::type FinalType;

 public:
  typedef boost::make_variant_over<IntermediateType>::type Messages;

 private:
  routing::Routing& routing_;
  AsioService asio_service_;
  nfs_client::DataGetter& data_getter_;
  Accumulator<Messages> accumulator_;
  routing::CloseNodesChange close_nodes_change_;
  MpidManagerDispatcher dispatcher_;
  MpidManagerDataBase db_;
  AccountTransferHandler<nfs::PersonaTypes<nfs::Persona::kMpidManager>> account_transfer_;
};

template <typename MessageType>
void MpidManagerService::HandleMessage(const MessageType& message,
                                       const typename MessageType::Sender& sender,
                                       const typename MessageType::Receiver& receiver) {}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MPID_MANAGER_MPID_MANAGER_HOLDER_SERVICE_H_
