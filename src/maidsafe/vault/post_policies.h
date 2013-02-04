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

#ifndef MAIDSAFE_VAULT_POST_POLICIES_H_
#define MAIDSAFE_VAULT_POST_POLICIES_H_

#include <string>
#include <vector>

#include "maidsafe/common/crypto.h"
#include "maidsafe/common/rsa.h"
#include "maidsafe/common/types.h"

#include "maidsafe/passport/types.h"

#include "maidsafe/routing/routing_api.h"

#include "maidsafe/nfs/message.h"
#include "maidsafe/nfs/response_mapper.h"
#include "maidsafe/nfs/types.h"


namespace maidsafe {

namespace vault {

template<nfs::Persona persona>
class PostSynchronisation {
 public:
  PostSynchronisation(nfs::NfsResponseMapper& /*response_mapper*/, routing::Routing& routing)
      : routing_(routing),
        source_(nfs::PersonaId(persona, routing.kNodeId())) {}

  explicit PostSynchronisation(routing::Routing& routing, const passport::Pmid& /*signing_pmid*/)
      : routing_(routing),
        source_(nfs::PersonaId(persona, routing.kNodeId())) {}

  void PostSyncData(const nfs::GenericMessage& generic_message,
                    nfs::GenericMessage::OnError /*on_error*/) {
    nfs::Message message(nfs::GenericMessage::message_type_identifier,
                         generic_message.Serialise().data);
    routing_.Send(NodeId(generic_message.name().string()), message.Serialise()->string(), false);
  }

 protected:
  ~PostSynchronisation() {}

 private:
  routing::Routing& routing_;
  nfs::PersonaId source_;
};

class PmidAccountHolderPolicy {
 public:
  PmidAccountHolderPolicy(nfs::NfsResponseMapper& /*response_mapper*/, routing::Routing& routing)
      : routing_(routing),
        source_(nfs::PersonaId(nfs::Persona::kPmidAccountHolder, routing.kNodeId())) {}
  void DataHolderStatusChanged(const NodeId& /*next_node*/, const NodeId& /*this_node*/, bool /*node_up*/) {
  }

 private:
  routing::Routing& routing_;
  nfs::PersonaId source_;
};

class MetadataManagerPolicy {
 public:
  MetadataManagerPolicy(nfs::NfsResponseMapper& /*response_mapper*/, routing::Routing& routing)
      : routing_(routing),
        source_(nfs::PersonaId(nfs::Persona::kMetadataManager, routing.kNodeId())) {}
  void DuplicateCopy();

 private:
  routing::Routing& routing_;
  nfs::PersonaId source_;
};

template<typename T, typename SyncPolicy>
class HostPost : public T, public SyncPolicy {
 public:
  explicit HostPost(nfs::NfsResponseMapper& response_mapper, routing::Routing& routing)
      : T(response_mapper, routing),
        SyncPolicy(response_mapper, routing) {}
};

typedef HostPost<PmidAccountHolderPolicy,
                 PostSynchronisation<nfs::Persona::kPmidAccountHolder> > PmidAccountHolderPost;
typedef HostPost<MetadataManagerPolicy,
                 PostSynchronisation<nfs::Persona::kMetadataManager> > MetadataManagerPost;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_POST_POLICIES_H_
