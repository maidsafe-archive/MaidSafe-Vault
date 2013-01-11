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
#include "maidsafe/nfs/types.h"


namespace maidsafe {

namespace vault {

class GetFromDataHolder {
 public:
  explicit GetFromDataHolder(routing::Routing& routing)
      : routing_(routing),
        source_(nfs::Message::Source(nfs::PersonaType::kMetadataManager, routing.kNodeId())) {}

  template<typename Data>
  std::future<Data> Get(const typename Data::name_type& name) {
    auto promise(std::make_shared<std::promise<Data>>());  // NOLINT (Fraser)
    std::future<Data> future(promise->get_future());
    routing::ResponseFunctor callback =
        [promise](const std::vector<std::string>& serialised_messages) {
          HandleGetResponse(promise, serialised_messages);
        };
    Message message(ActionType::kGet, PersonaType::kDataHolder, source_,
                    Data::name_type::tag_type::kEnumValue, name.data, NonEmptyString(),
                    asymm::Signature());
    routing_.Send(NodeId(name->string()), message.Serialise()->string(), callback,
                  routing::DestinationType::kGroup, IsCacheable<Data>());
    return std::move(future);
  }

 protected:
  ~GetFromDataHolder() {}

 private:
  routing::Routing& routing_;
  nfs::Message::Source source_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_GET_POLICIES_H_
