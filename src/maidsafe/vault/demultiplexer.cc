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

#include "maidsafe/vault/demultiplexer.h"

#include "maidsafe/common/log.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/data_types/data_type_values.h"
#include "maidsafe/data_types/data_name_variant.h"

namespace maidsafe {

namespace vault {

Demultiplexer::Demultiplexer(nfs::Service<MaidManagerService>& maid_manager_service,
                             nfs::Service<VersionHandlerService>& version_handler_service,
                             nfs::Service<DataManagerService>& data_manager_service,
                             nfs::Service<PmidManagerService>& pmid_manager_service,
                             nfs::Service<PmidNodeService>& pmid_node_service,
                             nfs_client::DataGetter& data_getter)
    : maid_manager_service_(maid_manager_service),
      version_handler_service_(version_handler_service),
      data_manager_service_(data_manager_service),
      pmid_manager_service_(pmid_manager_service),
      pmid_node_service_(pmid_node_service),
      data_getter_(data_getter) {}

}  // namespace vault

}  // namespace maidsafe
