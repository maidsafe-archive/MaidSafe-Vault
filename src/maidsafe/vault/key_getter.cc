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

#include "maidsafe/vault/key_getter.h"

#include "maidsafe/routing/routing_api.h"

namespace maidsafe {

namespace vault {

KeyGetter::KeyGetter(routing::Routing& /*routing*/,
                     const boost::filesystem::path /*vault_root_dir*/)
  : Routing_(Routing) {

}

void HandleGetKey(NodeId /*node_id*/, const routing::GivePublicKeyFunctor& give_key) {
  asymm::PublicKey key;  // get key from network for node id
  give_key(key);
}

}  // namespace vault

}  // namespace maidsafe
