/***************************************************************************************************
 *  Copyright 2012 MaidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of MaidSafe.net.                                  *
 **************************************************************************************************/

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

class MaidAccount;
class PmidAccount;

typedef nfs::NetworkFileSystem<
    nfs::MaidAccountHolderPutPolicy,
    nfs::MaidAccountHolderGetPolicy,
    nfs::MaidAccountHolderDeletePolicy,
    MaidAccountHolderPostPolicy> MaidAccountHolderNfs;

typedef nfs::NetworkFileSystem<
    nfs::MetadataManagerPutPolicy,
    nfs::MetadataManagerGetPolicy,
    nfs::MetadataManagerDeletePolicy,
    MetadataManagerPostPolicy> MetadataManagerNfs;

typedef nfs::NetworkFileSystem<
    nfs::PmidAccountHolderPutPolicy,
    nfs::PmidAccountHolderGetPolicy,
    nfs::PmidAccountHolderDeletePolicy,
    PmidAccountHolderPostPolicy> PmidAccountHolderNfs;

// TODO(dirvine) BEFORE_RELEASE this is a hack to create a type for the dataholder, the proper
// implmentation is required ,,,,,,,,,,,,,,,,,,,,,
typedef nfs::NetworkFileSystem<
    nfs::PmidAccountHolderPutPolicy,
    nfs::PmidAccountHolderGetPolicy,
    nfs::PmidAccountHolderDeletePolicy,
    PmidAccountHolderPostPolicy> DataHolderNfs;

typedef passport::PublicMaid::name_type MaidName;
typedef passport::PublicPmid::name_type PmidName;
typedef passport::PublicMpid::name_type MpidName;

typedef std::set<std::unique_ptr<MaidAccount>,
                 std::function<bool(const std::unique_ptr<MaidAccount>&,
                                    const std::unique_ptr<MaidAccount>&)>> MaidAccountSet;
typedef std::set<std::unique_ptr<PmidAccount>,
                 std::function<bool(const std::unique_ptr<PmidAccount>&,
                                    const std::unique_ptr<PmidAccount>&)>> PmidAccountSet;

typedef nfs::PersonaTypes<nfs::Persona::kStructuredDataManager> StructuredDataManager;



}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_TYPES_H_
