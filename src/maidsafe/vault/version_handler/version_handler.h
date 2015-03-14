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

#ifndef MAIDSAFE_VAULT_VERSION_HANDLER_H_
#define MAIDSAFE_VAULT_VERSION_HANDLER_H_

#include "maidsafe/common/types.h"
#include "maidsafe/common/data_types/structured_data_versions.h"
#include "maidsafe/routing/types.h"
#include "maidsafe/routing/source_address.h"

#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/version_handler/database.h"

namespace maidsafe {

namespace vault {

template <typename FacadeType>
class VersionHandler {
 public:
  VersionHandler(const boost::filesystem::path& vault_root_dir,
                 DiskUsage max_disk_usage);
  template <typename DataType>
  routing::HandleGetReturn HandleGet(routing::SourceAddress from, Identity data_name);

  bool HandlePut(const routing::SerialisedMessage& message);

  bool HandlePost(const routing::SerialisedMessage& message);

  void HandleChurn(routing::CloseGroupDifference);

 private:
  VersionHandlerDatabase db_;
};

template <typename FacadeType>
VersionHandler<FacadeType>::VersionHandler(const boost::filesystem::path& vault_root_dir,
                                           DiskUsage /*max_disk_usage*/)
  : db_(UniqueDbPath(vault_root_dir)) {}

}  // namespace vault

}  // namespace maidsafe

#endif // MAIDSAFE_VAULT_VERSION_HANDLER_H_
