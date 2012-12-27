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

#include "maidsafe/vault/public_key_getter.h"

#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/nfs.h"

namespace maidsafe {

namespace vault {

PublicKeyGetter::PublicKeyGetter(routing::Routing& routing)
    : routing_(routing),
      running_(true),
      pending_keys_(),
      flags_mutex_(),
      mutex_(),
      condition_(),
      thread_(new std::thread([this] { Run(); })) {
}

PublicKeyGetter::~PublicKeyGetter() {
  {
    std::lock_guard<std::mutex> flags_lock(flags_mutex_);
    running_ = false;
  }
  condition_.notify_one();
  thread_->join();
}

void PublicKeyGetter::HandleGetKey(const NodeId& /*node_id*/,
                             const routing::GivePublicKeyFunctor& give_key) {
//temp to be replaced by nfs get
std::promise<asymm::PublicKey> promise;
auto future = promise.get_future();
//
std::shared_ptr<PendingKey> pending_key(new PendingKey(std::move(future), give_key));
  AddPendingKey(pending_key);
}

void PublicKeyGetter::AddPendingKey(std::shared_ptr<PendingKey> pending_key) {
  std::lock_guard<std::mutex> flags_lock(flags_mutex_);
  if (!running_)
    return;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    pending_keys_.push_back(pending_key);
  }
  condition_.notify_one();
}

void PublicKeyGetter::Run() {
  auto running = [this]()->bool {
    std::lock_guard<std::mutex> flags_lock(flags_mutex_);
    return running_;
  };

  while (running()) {
    std::shared_ptr<PendingKey> ready_pending_key;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      while (pending_keys_.empty())
        condition_.wait(lock);

      auto itr_pending_key(pending_keys_.begin());
      while (itr_pending_key != pending_keys_.end()) {
        if (is_ready((*itr_pending_key)->future)) {
          ready_pending_key.swap(*itr_pending_key);
          pending_keys_.erase(itr_pending_key);
          break;
        }
      }
    }
    if (ready_pending_key) {
      asymm::PublicKey key = ready_pending_key->future.get();
      ready_pending_key->give_key(key);
      ready_pending_key.reset();
    }
  }
}

}  // namespace vault

}  // namespace maidsafe
