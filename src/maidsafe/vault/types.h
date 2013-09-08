/*  Copyright 2012 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.novinet.com/license

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#ifndef MAIDSAFE_VAULT_TYPES_H_
#define MAIDSAFE_VAULT_TYPES_H_

#include <functional>
#include <memory>
#include <set>

#include "maidsafe/common/tagged_value.h"
#include "maidsafe/common/types.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/nfs/types.h"


namespace maidsafe {

namespace vault {

namespace detail {

enum class DataType : int32_t { kMetadata = 10000 };

}  // namespace detail

//class MaidAccount;
//class PmidAccount;
//
//typedef nfs::NetworkFileSystem<
//    nfs::MaidManagerPutPolicy,
//    nfs::MaidManagerGetPolicy,
//    nfs::MaidManagerDeletePolicy,
//    MaidManagerPostPolicy> MaidManagerNfs;
//
//typedef nfs::NetworkFileSystem<
//    nfs::DataManagerPutPolicy,
//    nfs::DataManagerGetPolicy,
//    nfs::DataManagerDeletePolicy,
//    DataManagerPostPolicy> DataManagerNfs;
//
//typedef nfs::NetworkFileSystem<
//    nfs::VersionManagerPutPolicy,
//    nfs::VersionManagerGetPolicy,
//    nfs::VersionManagerDeletePolicy,
//    VersionManagerPostPolicy> VersionManagerNfs;
//
//typedef nfs::NetworkFileSystem<
//    nfs::PmidManagerPutPolicy,
//    nfs::PmidManagerGetPolicy,
//    nfs::PmidManagerDeletePolicy,
//    PmidManagerPostPolicy> PmidManagerNfs;
//
//typedef nfs::NetworkFileSystem<
//    nfs::PmidNodePutPolicy,
//    nfs::PmidNodeGetPolicy,
//    nfs::PmidNodeDeletePolicy,
//    PmidNodePostPolicy> PmidNodeNfs;

typedef passport::PublicMaid::Name MaidName;
typedef passport::PublicPmid::Name PmidName;
typedef passport::PublicMpid::Name MpidName;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_TYPES_H_
