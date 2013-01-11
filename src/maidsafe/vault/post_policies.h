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

#ifndef MAIDSAFE_NFS_POST_POLICIES_H_
#define MAIDSAFE_NFS_POST_POLICIES_H_

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

template<typename SigningFob>
class NoPost {
 public:
  NoPost() {}
  explicit NoPost(routing::Routing& /*routing*/) {}
  explicit NoPost(routing::Routing& /*routing*/, const SigningFob& /*signing_fob*/) {}

  template<typename Data>
  void Post(const typename Data::name_type& /*name*/) {}

 protected:
  ~NoPost() {}
};

template<PersonaType persona>
class PostSynchronisation {
 public:
  PostSynchronisation(routing::Routing& routing, const passport::Pmid& /*signing_pmid*/)
      : routing_(routing),
        source_(Message::Source(persona, routing.kNodeId())) {}

  void PostSyncData(const PostMessage& message, OnPostError on_error) {
    PostMessage new_message(message.post_action_type(),
                            message.destination_persona_type(),
                            source_,
                            message.name(),
                            message.content(),
                            maidsafe::rsa::Signature());

    routing::ResponseFunctor callback =
        [on_error, new_message](const std::vector<std::string>& serialised_messages) {
          HandlePostResponse(on_error, new_message, serialised_messages);
        };
    routing_.Send(NodeId(new_message.name().string()), message.Serialise()->string(),
                  callback, routing::DestinationType::kGroup, false);
  }

 protected:
  ~PostSynchronisation() {}

 private:
  routing::Routing& routing_;
  Message::Source source_;
};

}  // namespace nfs

}  // namespace maidsafe

#endif  // MAIDSAFE_NFS_POST_POLICIES_H_
