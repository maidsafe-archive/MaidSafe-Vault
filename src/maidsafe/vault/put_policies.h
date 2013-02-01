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
#include "maidsafe/nfs/response_mapper.h"
#include "maidsafe/nfs/reply.h"
#include "maidsafe/nfs/types.h"


namespace maidsafe {

namespace vault {

class PutToMetadataManager {
 public:
  PutToMetadataManager(routing::Routing& routing, const passport::Pmid& signing_pmid,
                       nfs::ResponseMapper& response_mapper)
      : routing_(routing),
        signing_pmid_(signing_pmid),
        response_mapper_(response_mapper),
        source_(nfs::PersonaId(nfs::Persona::kMaidAccountHolder, routing.kNodeId())) {}

  template<typename Data>
  std::vector<std::future<nfs::Reply>> Put(const nfs::DataMessage& data_message) {
    nfs::DataMessage new_data_message(
        data_message.next_persona(),
        source_,
        nfs::DataMessage::Data(data_message.data().type,
                               data_message.data().name,
                               data_message.data().content,
                               data_message.data().action));
    nfs::Message message(nfs::DataMessage::message_type_identifier,
                         new_data_message.Serialise().data);
    auto routing_futures(std::make_shared<std::vector<std::future<std::string>>>(
        routing_.SendGroup(NodeId(new_data_message.data().name.string()),
                           message.Serialise()->string(),
                           nfs::IsCacheable<Data>())));
    return GetMappedNfsFutures(std::move(routing_futures), response_mapper_);
  }

 protected:
  ~PutToMetadataManager() {}

 private:
  routing::Routing& routing_;
  passport::Pmid signing_pmid_;
  nfs::ResponseMapper& response_mapper_;
  nfs::PersonaId source_;
};

class PutToPmidAccountHolder {
 public:
  explicit PutToPmidAccountHolder(routing::Routing& routing, nfs::ResponseMapper& response_mapper)
      : routing_(routing),
        response_mapper_(response_mapper),
        source_(nfs::PersonaId(nfs::Persona::kMetadataManager, routing.kNodeId())) {}

  template<typename Data>
   std::vector<std::future<nfs::Reply>> Put(const nfs::DataMessage& data_message) {
    nfs::DataMessage new_data_message(
        data_message.next_persona(),
        source_,
        nfs::DataMessage::Data(data_message.data().type,
                               data_message.data().name,
                               data_message.data().content,
                               data_message.data().action));
    nfs::Message message(nfs::DataMessage::message_type_identifier,
                         new_data_message.Serialise().data);
    auto routing_futures(std::make_shared<std::vector<std::future<std::string>>>(
        routing_.SendGroup(NodeId(new_data_message.data().name.string()),
                           message.Serialise()->string(),
                           nfs::IsCacheable<Data>())));

    return GetMappedNfsFutures(std::move(routing_futures), response_mapper_);
  }

 protected:
  ~PutToPmidAccountHolder() {}

 private:
  routing::Routing& routing_;
  nfs::ResponseMapper& response_mapper_;
  nfs::PersonaId source_;
};

class PutToDataHolder {
 public:
  explicit PutToDataHolder(routing::Routing& routing, nfs::ResponseMapper& response_mapper)
      : routing_(routing),
        response_mapper_(response_mapper),
        source_(nfs::PersonaId(nfs::Persona::kPmidAccountHolder, routing.kNodeId())) {}

  template<typename Data>
  std::vector<std::future<nfs::Reply>> Put(const nfs::DataMessage& data_message) {
    nfs::DataMessage new_data_message(
        data_message.next_persona(),
        source_,
        nfs::DataMessage::Data(data_message.data().type,
                               data_message.data().name,
                               data_message.data().content,
                               data_message.data().action));
    nfs::Message message(nfs::DataMessage::message_type_identifier,
                         new_data_message.Serialise().data);
    auto routing_futures(std::make_shared<std::vector<std::future<std::string>>>(
        routing_.SendGroup(NodeId(new_data_message.data().name.string()),
                           message.Serialise()->string(),
                           nfs::IsCacheable<Data>())));

    return GetMappedNfsFutures(std::move(routing_futures), response_mapper_);
  }

 protected:
  ~PutToDataHolder() {}

 private:
  routing::Routing& routing_;
  nfs::ResponseMapper& response_mapper_;
  nfs::PersonaId source_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_PUT_POLICIES_H_
