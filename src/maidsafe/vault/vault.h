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

#ifndef MAIDSAFE_VAULT_VAULT_H_
#define MAIDSAFE_VAULT_VAULT_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "boost/filesystem/path.hpp"
#include "boost/date_time/posix_time/ptime.hpp"
#include "boost/thread/mutex.hpp"

#include "maidsafe/common/asio_service.h"
#include "maidsafe/common/rsa.h"

#include "maidsafe/nfs/nfs.h"

#include "maidsafe/vault/demultiplexer.h"
#include "maidsafe/vault/meta_data_manager.h"
#include "maidsafe/vault/pmid_account_holder.h"
#include "maidsafe/vault/maid_account_holder.h"
#include "maidsafe/vault/data_holder.h"


namespace maidsafe {

namespace vault {

typedef NetworkFileSystem<GetFromMetaDataManager,
        PutToMetaDataManager,
        PostToDirect,
        template<DeleteLocal, DeleteFromMetaDataManager>>PmidAccountHolderNfs;

class Vault {
 public:
#ifdef TESTING
  Vault(Pmid pmid, boost::filesystem::path vault_root_dir);
#endif
  Vault();
  ~Vault();  // must issue StopSending() to all identity objects (MM etc.)
            // Then ensure routing is destroyed next then allothers in ny order at this time
 private:
  std::unique_ptr<routing::Routing> routing_;
  Demultiplexer demux_;
  MaidAccountHolder maid_account_holder_;
  PmidAccountHolder pmid_account_holder_;
  MetaDataManager meta_data_manager_;
  DataHolder data_holder_;
};



}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VAULT_H_
