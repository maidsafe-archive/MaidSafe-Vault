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

#include "maidsafe/vault/data_holder.h"
#include "boost/filesystem/path.hpp"
#include "boost/filesystem/filesystem.hpp"
#include "maidsafe/nfs/message.h"

namespace maidsafe {

namespace vault {

DataHolder::DataHolder(routing::Routing& routing, const boost::filesystem::path vault_root_dir)
    : persona_dir_(vault_root_dir + "data_holder") {
      boost::filesystem::exists(persona_dir_) ||  boost::filesystem::create_directory(persona_dir);
}

void DataHolder::HandleMessage(const nfs::Message& message,
                               routing::ReplyFunctor /*reply_functor*/) {
  switch (message.action_type()) {
    case nfs::ActionType::kGet :
      HandleGetMessage(message);
      break;
    case nfs::ActionType::kPut :
      HandlePutMessage(message);
      break;
    case nfs::ActionType::kPost :
      HandlePostMessage(message);
      break;
    case nfs::ActionType::kDelete :
      HandleDeleteMessage(message);
      break;
    default :
      LOG(kError) << "Unhandled action type";
  }
}

bool DataHolder::HaveCache(nfs::Message& message,
                           const routing::Routing& routing,
                           const DaraStore& data_store) {
  return false;
  //return disk_storage.Find(message.signature()); // TODO:(Team):FIXME
}

void DataHolder::StoreCache(const nfs::Message& /*message*/,
                            const routing::Routing& /*routing*/,
                            const DiskBasedStorage& /*disk_storage*/) {
//  disk_storage.Save(message.signature());
}

void DataHolder::StopSending() {
}

}  // namespace vault

}  // namespace maidsafe
