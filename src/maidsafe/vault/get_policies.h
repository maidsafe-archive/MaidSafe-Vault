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

#ifndef MAIDSAFE_NFS_GET_POLICIES_H_
#define MAIDSAFE_NFS_GET_POLICIES_H_

#include <algorithm>
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

class GetFromPmidAccountHolder {
 public:
  explicit GetFromPmidAccountHolder(const routing::Routing&);
};


class NoGet {
 public:
  NoGet() {}
  explicit NoGet(routing::Routing& /*routing*/) {}

  template<typename Data>
  std::future<Data> Get(const typename Data::name_type& /*name*/) {}

 protected:
  ~NoGet() {}
};


template<PersonaType persona>
class GetFromMetadataManager {
 public:
  explicit GetFromMetadataManager(routing::Routing& routing)
      : routing_(routing),
        source_(Message::Source(persona, routing.kNodeId())) {}

  template<typename Data>
  std::future<Data> Get(const typename Data::name_type& name) {
    auto promise(std::make_shared<std::promise<Data>>());  // NOLINT (Fraser)
    std::future<Data> future(promise->get_future());
    routing::ResponseFunctor callback =
        [promise](const std::vector<std::string>& serialised_messages) {
          HandleGetResponse(promise, serialised_messages);
        };
    Message message(ActionType::kGet, PersonaType::kMetadataManager, source_,
                    Data::name_type::tag_type::kEnumValue, name.data, NonEmptyString(),
                    asymm::Signature());
    routing_.Send(NodeId(name->string()), message.Serialise()->string(), callback,
                  routing::DestinationType::kGroup, IsCacheable<Data>());
    return std::move(future);
  }

 protected:
  ~GetFromMetadataManager() {}

 private:
  routing::Routing& routing_;
  Message::Source source_;
};

template<PersonaType persona>
class GetFromDataHolder {
 public:
  explicit GetFromDataHolder(routing::Routing& routing)
      : routing_(routing),
        source_(Message::Source(persona, routing.kNodeId())) {}

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
  Message::Source source_;
};

template<PersonaType persona>
class GetFromMaidAccountHolder {
 public:
  explicit GetFromMaidAccountHolder(routing::Routing& routing)
      : routing_(routing),
        source_(Message::Source(persona, routing.kNodeId())) {}

  template<typename Data>
  std::future<Data> Get(const typename Data::name_type& name) {
    auto promise(std::make_shared<std::promise<Data>>());  // NOLINT (Fraser)
    std::future<Data> future(promise->get_future());
    routing::ResponseFunctor callback =
        [promise](const std::vector<std::string>& serialised_messages) {
          HandleGetResponse(promise, serialised_messages);
        };
    Message message(ActionType::kGet, PersonaType::kMaidAccountHolder, source_,
                    Data::name_type::tag_type::kEnumValue, name.data, NonEmptyString(),
                    asymm::Signature());
    routing_.Send(NodeId(name->string()), message.Serialise()->string(), callback,
                  routing::DestinationType::kGroup, IsCacheable<Data>());
    return std::move(future);
  }

 protected:
  ~GetFromMaidAccountHolder() {}

 private:
  routing::Routing& routing_;
  Message::Source source_;
};


#ifdef TESTING

class GetFromKeyFile {
 public:
  explicit GetFromKeyFile(const std::vector<passport::PublicPmid>& all_pmids)
      : kAllPmids_(all_pmids) {}
  std::future<passport::PublicPmid> Get(const passport::PublicPmid::name_type& name);

 private:
  const std::vector<passport::PublicPmid> kAllPmids_;
};

#endif


}  // namespace nfs

}  // namespace maidsafe

#endif  // MAIDSAFE_NFS_GET_POLICIES_H_
