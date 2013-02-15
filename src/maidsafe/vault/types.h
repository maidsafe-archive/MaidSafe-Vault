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

#include "maidsafe/common/tagged_value.h"
#include "maidsafe/common/types.h"

#include "maidsafe/nfs/data_policies.h"
#include "maidsafe/nfs/nfs.h"

#include "maidsafe/passport/types.h"


#include "maidsafe/vault/put_policies.h"
#include "maidsafe/vault/post_policies.h"
#include "maidsafe/vault/delete_policies.h"


namespace maidsafe {

namespace vault {

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

// TODO (dirvine) BEFORE_RELEASE this is a hack to create a type for the dataholder, the proper implmentation is required ,,,,,,,,,,,,,,,,,,,,,
typedef nfs::NetworkFileSystem<
    nfs::PmidAccountHolderPutPolicy,
    nfs::PmidAccountHolderGetPolicy,
    nfs::PmidAccountHolderDeletePolicy,
    PmidAccountHolderPostPolicy> DataHolderNfs;

typedef passport::PublicMaid::name_type MaidName;
typedef passport::PublicPmid::name_type PmidName;
typedef passport::PublicMpid::name_type MpidName;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_TYPES_H_
