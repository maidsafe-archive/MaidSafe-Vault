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

#include "maidsafe/vault/demultiplexer.h"

#include "maidsafe/common/log.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/data_types/data_type_values.h"
#include "maidsafe/data_types/data_name_variant.h"
//#include "maidsafe/nfs/reply.h"

#include "maidsafe/vault/pmid_node/service.h"
#include "maidsafe/vault/maid_manager/service.h"
#include "maidsafe/vault/data_manager/service.h"
#include "maidsafe/vault/pmid_manager/service.h"
#include "maidsafe/vault/version_manager/service.h"


namespace maidsafe {

namespace vault {

Demultiplexer::Demultiplexer(MaidManagerService& maid_manager_service,
                             VersionManagerService& version_manager_service,
                             DataManagerService& data_manager_service,
                             PmidManagerService& pmid_manager_service,
                             PmidNodeService& pmid_node)
    : maid_manager_service_(maid_manager_service),
      version_manager_service_(version_manager_service),
      data_manager_service_(data_manager_service),
      pmid_manager_service_(pmid_manager_service),
      pmid_node_(pmid_node) {}

}  // namespace vault

}  // namespace maidsafe
