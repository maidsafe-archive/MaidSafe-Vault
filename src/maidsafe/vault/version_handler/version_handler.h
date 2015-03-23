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

#ifndef MAIDSAFE_VAULT_VERSION_HANDLER_VERSION_HANDLER_H_
#define MAIDSAFE_VAULT_VERSION_HANDLER_VERSION_HANDLER_H_

#include <string>

#include "maidsafe/common/convert.h"
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

  routing::HandleGetReturn HandleGet(const routing::SourceAddress& from, const Identity& sdv_name);

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

template <typename FacadeType>
routing::HandleGetReturn VersionHandler<FacadeType>::HandleGet(
    const routing::SourceAddress& /* from */, const Identity& sdv_name) {
  try {
    std::string serialised_sdv;
    std::string key(convert::ToString(sdv_name.string()));
    db_.Get(key, serialised_sdv);
    return routing::HandleGetReturn::value_type(convert::ToByteVector(serialised_sdv));
  } catch (const maidsafe_error& error) {
    return boost::make_unexpected(error);
  } catch (...) {
    return boost::make_unexpected(MakeError(CommonErrors::unable_to_handle_request));
  }
}

template <typename FacadeType>
bool VersionHandler<FacadeType>::HandlePut(const routing::SerialisedMessage& message) {
  InputVectorStream binary_input_stream { message };
  Identity sdv_name;
  StructuredDataVersions::VersionName version;
  uint32_t max_versions, max_branches;
  Parse(binary_input_stream, sdv_name, version, max_versions, max_branches);
  std::string key(convert::ToString(sdv_name.string()));
  try {
    std::string serialised_sdv;
    db_.Get(key, serialised_sdv);
    return false;
  } catch (const maidsafe_error& error) {
    if (error.code() != make_error_code(VaultErrors::no_such_account))
      return false;
  }
  StructuredDataVersions sdv(max_versions, max_branches);
  sdv.Put(StructuredDataVersions::VersionName(), version);
  db_.Put(key, convert::ToString(sdv.Serialise().data.string()));
  return true;
}

template <typename FacadeType>
bool VersionHandler<FacadeType>::HandlePost(const routing::SerialisedMessage& message) {
  InputVectorStream binary_input_stream { message };
  Identity sdv_name;
  StructuredDataVersions::VersionName new_version, old_version;
  Parse(binary_input_stream, sdv_name, old_version, new_version);
  std::string key(convert::ToString(sdv_name.string()));
  try {
    std::string serialised_sdv;
    db_.Get(key, serialised_sdv);
    MutableData sdv_wrapper(Parse<MutableData>(convert::ToByteVector(serialised_sdv)));
    StructuredDataVersions sdv(20, 1);
    sdv.ApplySerialised(StructuredDataVersions::serialised_type(sdv_wrapper.Value()));
    sdv.Put(old_version, new_version);
    db_.Put(key, convert::ToString(sdv.Serialise().data.string()));
  } catch (...) {
    return false;
  }
  return true;
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VERSION_HANDLER_VERSION_HANDLER_H_
