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

#ifndef MAIDSAFE_VAULT_KEY_GETTER_H_
#define MAIDSAFE_VAULT_KEY_GETTER_H_

#include <chrono>
#include <future>
#include <string>

#include "maidsafe/routing/api_config.h"



namespace maidsafe {

namespace routing { class Routing; }

namespace vault {

class PublicKeyGetter {
 public:
  PublicKeyGetter(routing::Routing& routing);
  ~PublicKeyGetter();
  void HandleGetKey(const NodeId& node_id, const routing::GivePublicKeyFunctor& give_key);

 private:
  struct PendingKey {
    PendingKey(std::future<asymm::PublicKey> future_in, routing::GivePublicKeyFunctor give_key_in)
        : future(std::move(future_in)),
          give_key(give_key_in) {
    }
    std::future<asymm::PublicKey> future;
    routing::GivePublicKeyFunctor give_key;
  };
  void Run();
  void AddPendingKey(std::shared_ptr<PendingKey> pending_key);

  routing::Routing& routing_;
  bool running_;
  std::vector<std::shared_ptr<PendingKey>> pending_keys_;
  std::mutex flags_mutex_, mutex_;
  std::condition_variable condition_;
  std::shared_ptr<std::thread> thread_;
};

template<typename Future>
bool is_ready(std::future<Future>& f) {
  return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_KEY_GETTER_H_
