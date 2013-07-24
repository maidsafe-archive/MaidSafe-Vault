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

#ifndef MAIDSAFE_VAULT_TYPES_H_
#define MAIDSAFE_VAULT_TYPES_H_

#include <functional>
#include <memory>
#include <set>

#include "maidsafe/common/tagged_value.h"
#include "maidsafe/common/types.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/nfs/data_policies.h"
#include "maidsafe/nfs/nfs.h"
#include "maidsafe/nfs/types.h"

#include "maidsafe/vault/post_policies.h"


namespace maidsafe {

namespace vault {

namespace detail {

enum class DataType : int32_t { kMetadata = 10000 };

}  // namespace detail

class MaidAccount;
class PmidAccount;

typedef nfs::NetworkFileSystem<
    nfs::MaidManagerPutPolicy,
    nfs::MaidManagerGetPolicy,
    nfs::MaidManagerDeletePolicy,
    MaidManagerPostPolicy> MaidManagerNfs;

typedef nfs::NetworkFileSystem<
    nfs::DataManagerPutPolicy,
    nfs::DataManagerGetPolicy,
    nfs::DataManagerDeletePolicy,
    DataManagerPostPolicy> DataManagerNfs;

typedef nfs::NetworkFileSystem<
    nfs::VersionManagerPutPolicy,
    nfs::VersionManagerGetPolicy,
    nfs::VersionManagerDeletePolicy,
    VersionManagerPostPolicy> VersionManagerNfs;

typedef nfs::NetworkFileSystem<
    nfs::PmidManagerPutPolicy,
    nfs::PmidManagerGetPolicy,
    nfs::PmidManagerDeletePolicy,
    PmidManagerPostPolicy> PmidManagerNfs;

// TODO(dirvine) BEFORE_RELEASE this is a hack to create a type for the dataholder, the proper
// implmentation is required ,,,,,,,,,,,,,,,,,,,,,
typedef nfs::NetworkFileSystem<
    nfs::PmidManagerPutPolicy,
    nfs::PmidManagerGetPolicy,
    nfs::PmidManagerDeletePolicy,
    PmidManagerPostPolicy> PmidNodeNfs;

typedef passport::PublicMaid::name_type MaidName;
typedef passport::PublicPmid::name_type PmidName;
typedef passport::PublicMpid::name_type MpidName;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_TYPES_H_
