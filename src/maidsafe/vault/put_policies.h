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

#ifndef MAIDSAFE_VAULT_PUT_POLICIES_H_
#define MAIDSAFE_VAULT_PUT_POLICIES_H_

#include <string>
#include <vector>

#include "maidsafe/common/crypto.h"
#include "maidsafe/common/rsa.h"
#include "maidsafe/common/types.h"

#include "maidsafe/passport/types.h"

#include "maidsafe/routing/routing_api.h"

#include "maidsafe/nfs/message.h"
#include "maidsafe/nfs/types.h"


namespace maidsafe {

namespace vault {

class PutToMetadataManager {
 public:
  PutToMetadataManager(routing::Routing& routing, const passport::Pmid& signing_pmid)
      : routing_(routing),
        signing_pmid_(signing_pmid),
        source_(nfs::Message::Source(nfs::PersonaType::kMaidAccountHolder, routing.kNodeId())) {}

  template<typename Data>
  void Put(const nfs::Message& message, nfs::OnError on_error) {
    nfs::Message new_message(message.action_type(),
                             message.destination_persona_type(),
                             source_,
                             message.data_type(),
                             message.name(),
                             message.content(),
                             asymm::Sign(message.content(), signing_pmid_.private_key()));

    routing::ResponseFunctor callback =
        [on_error, new_message](const std::vector<std::string>& serialised_messages) {
          HandlePutResponse<Data>(on_error, new_message, serialised_messages);
        };
    routing_.Send(NodeId(new_message.name().string()), message.Serialise()->string(),
                  callback, routing::DestinationType::kGroup, IsCacheable<Data>());
  }

 protected:
  ~PutToMetadataManager() {}

 private:
  routing::Routing& routing_;
  passport::Pmid signing_pmid_;
  nfs::Message::Source source_;
};

class PutToPmidAccountHolder {
 public:
  explicit PutToPmidAccountHolder(routing::Routing& routing)
      : routing_(routing),
        source_(nfs::Message::Source(nfs::PersonaType::kMetadataManager, routing.kNodeId())) {}

  template<typename Data>
  void Put(const nfs::Message& message, nfs::OnError on_error) {
    nfs::Message new_message(message.action_type(),
                             message.destination_persona_type(),
                             source_,
                             message.data_type(),
                             message.name(),
                             message.content(),
                             message.signature());

    routing::ResponseFunctor callback =
        [on_error, new_message](const std::vector<std::string>& serialised_messages) {
          HandlePutResponse<Data>(on_error, new_message, serialised_messages);
        };
    routing_.Send(routing_.GetRandomExistingNode(), message.Serialise()->string(),
                  callback, routing::DestinationType::kGroup, IsCacheable<Data>());
  }

 protected:
  ~PutToPmidAccountHolder() {}

 private:
  routing::Routing& routing_;
  nfs::Message::Source source_;
};

class PutToDataHolder {
 public:
  PutToDataHolder(routing::Routing& routing)
      : routing_(routing),
        source_(nfs::Message::Source(nfs::PersonaType::kPmidAccountHolder, routing.kNodeId())) {}

  template<typename Data>
  void Put(const Data& data, nfs::OnError on_error) {
    NonEmptyString content(data.Serialise());
    nfs::Message message(ActionType::kPut,
                         PersonaType::kDataHolder,
                         source_,
                         Data::name_type::tag_type::kEnumValue,
                         data.name(),
                         content,
                         asymm::Sign(content, signing_fob_.private_key()));

    routing::ResponseFunctor callback =
        [on_error, message](const std::vector<std::string>& serialised_messages) {
          HandlePutResponse<Data>(on_error, message, serialised_messages);
        };
    routing_.Send(NodeId(data.name()->string()), message.Serialise()->string(), callback,
                  routing::DestinationType::kGroup, IsCacheable<Data>());
  }

 protected:
  ~PutToDataHolder() {}

 private:
  routing::Routing& routing_;
  nfs::Message::Source source_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PUT_POLICIES_H_
