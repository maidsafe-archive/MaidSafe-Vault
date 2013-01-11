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

#ifndef MAIDSAFE_NFS_DELETE_POLICIES_H_
#define MAIDSAFE_NFS_DELETE_POLICIES_H_

#include <future>
#include <string>
#include <vector>

#include "maidsafe/common/rsa.h"
#include "maidsafe/common/crypto.h"
#include "maidsafe/common/types.h"

#include "maidsafe/passport/types.h"

#include "maidsafe/routing/routing_api.h"

#include "maidsafe/nfs/utils.h"


namespace maidsafe {

namespace nfs {

class DeleteFromPmidAccountHolder {
 public:
  explicit DeleteFromPmidAccountHolder(const routing::Routing&);
};


template<typename SigningFob>
class NoDelete {
 public:
  NoDelete() {}
  NoDelete(routing::Routing& routing, const SigningFob& signing_fob)
      : routing_(routing),
        signing_fob_(signing_fob) {}
  template<typename Data>
  void Delete(const Message& /*message*/, OnError /*on_error*/) {}

 protected:
  ~NoDelete() {}

 private:
  routing::Routing& routing_;
  SigningFob signing_fob_;
};

class DeleteFromMetadataManager {
 public:
  DeleteFromMetadataManager(routing::Routing& routing, const passport::Pmid& signing_pmid)
    : routing_(routing),
      signing_pmid_(signing_pmid),
      source_(Message::Source(PersonaType::kMaidAccountHolder, routing.kNodeId())) {}

  template<typename Data>
  void Delete(const Message& message, OnError on_error) {
    Message new_message(message.action_type(),
                        message.destination_persona_type(),
                        source_,
                        message.data_type(),
                        message.name(),
                        message.content(),
                        asymm::Sign(message.content(), signing_pmid_.private_key()));

    routing::ResponseFunctor callback =
        [on_error, new_message](const std::vector<std::string>& serialised_messages) {
          HandleDeleteResponse<Data>(on_error, new_message, serialised_messages);
        };
    routing_.Send(NodeId(new_message.name().string()), message.Serialise()->string(),
                  callback, routing::DestinationType::kGroup, IsCacheable<Data>());
  }

 protected:
  ~DeleteFromMetadataManager() {}

 private:
  routing::Routing& routing_;
  passport::Pmid signing_pmid_;
  Message::Source source_;
};

}  // namespace nfs

}  // namespace maidsafe

#endif  // MAIDSAFE_NFS_DELETE_POLICIES_H_
