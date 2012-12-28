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

#ifndef MAIDSAFE_VAULT_META_DATA_MANAGER_H_
#define MAIDSAFE_VAULT_META_DATA_MANAGER_H_

#include "boost/filesystem.hpp"

#include "maidsafe/routing/api_config.h"

namespace maidsafe {

namespace nfs { class Message; }
namespace routing { class Routing; }

namespace vault {

class MetadataManager {
 public:
  MetadataManager(routing::Routing& routing, const boost::filesystem::path vault_root_dir);

  template <typename Data>
  void HandleMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);

 private:
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_META_DATA_MANAGER_H_
