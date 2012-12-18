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
namespace maidsafe {

namespace vault {

DataHolder::DataHolder(/*routing::Routing& routing*/)
    : vault_root_dir_() {

}

void DataHolder::HandleMessage(const nfs::Message& /*message*/,
                               routing::ReplyFunctor /*reply_functor*/) {
}

bool DataHolder::HaveCache(nfs::Message& /*message*/) {
  return false;
}

void DataHolder::StoreCache(const nfs::Message& /*message*/) {
}

void DataHolder::StopSending() {
}

}  // namespace vault

}  // namespace maidsafe
