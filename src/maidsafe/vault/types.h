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

#include "boost/variant/variant.hpp"

#include "maidsafe/common/tagged_value.h"
#include "maidsafe/common/types.h"

#include "maidsafe/nfs/nfs.h"

#include "maidsafe/passport/types.h"
#include "maidsafe/data_types/immutable_data.h"
#include "maidsafe/data_types/mutable_data.h"

#include "maidsafe/vault/get_policies.h"
#include "maidsafe/vault/put_policies.h"
#include "maidsafe/vault/post_policies.h"
#include "maidsafe/vault/delete_policies.h"


namespace maidsafe {

namespace vault {

typedef nfs::NetworkFileSystem<nfs::NoGet,
                               PutToMetadataManager,
                               PostSynchronisation<nfs::Persona::kMaidAccountHolder>,
                               DeleteFromMetadataManager> MaidAccountHolderNfs;

typedef nfs::NetworkFileSystem<GetFromDataHolder,
                               PutToPmidAccountHolder,
                               PostSynchronisation<nfs::Persona::kMetadataManager>,
                               DeleteFromPmidAccountHolder> MetadataManagerNfs;

typedef nfs::NetworkFileSystem<nfs::NoGet,
                               PutToDataHolder,
                               PostSynchronisation<nfs::Persona::kPmidAccountHolder>,
                               DeleteFromDataHolder> PmidAccountHolderNfs;

typedef passport::PublicMaid::name_type MaidName;
typedef passport::PublicPmid::name_type PmidName;
typedef passport::PublicMpid::name_type MpidName;

typedef boost::variant<passport::PublicAnmid::name_type,
                       passport::PublicAnsmid::name_type,
                       passport::PublicAntmid::name_type,
                       passport::PublicAnmaid::name_type,
                       passport::PublicMaid::name_type,
                       passport::PublicPmid::name_type,
                       passport::Mid::name_type,
                       passport::Smid::name_type,
                       passport::Tmid::name_type,
                       passport::PublicAnmpid::name_type,
                       passport::PublicMpid::name_type,
                       ImmutableData::name_type,
                       MutableData::name_type> DataNameVariant;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_TYPES_H_
