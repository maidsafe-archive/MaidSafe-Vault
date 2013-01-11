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

#include "maidsafe/nfs/nfs.h"

#include "maidsafe/vault/get_policies.h"
#include "maidsafe/vault/put_policies.h"
#include "maidsafe/vault/post_policies.h"
#include "maidsafe/vault/delete_policies.h"


namespace maidsafe {

namespace vault {

typedef nfs::NetworkFileSystem<nfs::NoGet,
                               PutToMetadataManager,
                               PostSynchronisation<nfs::PersonaType::kMaidAccountHolder>,
                               DeleteFromMetadataManager> MaidAccountHolderNfs;

typedef nfs::NetworkFileSystem<GetFromDataHolder,
                               PutToPmidAccountHolder,
                               PostSynchronisation<nfs::PersonaType::kMetadataManager>,
                               DeleteFromPmidAccountHolder> MetadataManagerNfs;

typedef nfs::NetworkFileSystem<nfs::NoGet,
                               PutToDataHolder,
                               PostSynchronisation<nfs::PersonaType::kPmidAccountHolder>,
                               DeleteFromDataHolder> PmidAccountHolderNfs;


}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_TYPES_H_
