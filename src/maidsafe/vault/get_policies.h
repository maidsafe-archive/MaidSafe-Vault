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

#ifndef MAIDSAFE_VAULT_GET_POLICIES_H_
#define MAIDSAFE_VAULT_GET_POLICIES_H_

#include <future>
#include <memory>
#include <string>
#include <vector>

#include "maidsafe/common/rsa.h"
#include "maidsafe/common/types.h"

#include "maidsafe/routing/routing_api.h"

#include "maidsafe/nfs/message.h"
#include "maidsafe/nfs/response_mapper.h"
#include "maidsafe/nfs/types.h"


namespace maidsafe {

namespace vault {

class GetFromDataHolder {
 public:
  GetFromDataHolder(routing::Routing& routing, nfs::ResponseMapper& response_mapper)
      : routing_(routing),
        response_mapper_(response_mapper),
        source_(nfs::PersonaId(nfs::Persona::kMetadataManager, routing.kNodeId())) {}

  template<typename Data>
  std::vector<std::future<nfs::ReturnCode>> Get(const typename Data::name_type& name,
                                                const Identity& dest_id) {
    auto promise(std::make_shared<std::promise<Data>>());
    std::future<Data> future(promise->get_future());
    nfs::DataMessage data_message(
        nfs::DataMessage::Action::kGet,
        nfs::Persona::kDataHolder,
        source_,
        nfs::DataMessage::Data(Data::name_type::tag_type::kEnumValue, name.data, NonEmptyString()));
    nfs::Message message(nfs::DataMessage::message_type_identifier, data_message.Serialise().data);
    auto routing_futures(std::make_shared<std::vector<std::future<std::string>>>(
        routing_.SendGroup(NodeId(dest_id), message.Serialise()->string(),
                           nfs::IsCacheable<Data>())));
    return GetMappedNfsFutures(std::move(routing_futures), response_mapper_);
  }

 protected:
  ~GetFromDataHolder() {}

 private:
  routing::Routing& routing_;
  nfs::ResponseMapper& response_mapper_;
  nfs::PersonaId source_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_GET_POLICIES_H_
