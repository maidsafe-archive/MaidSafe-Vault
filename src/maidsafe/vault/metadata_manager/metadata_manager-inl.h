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

#ifndef MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_MANAGER_INL_H_
#define MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_MANAGER_INL_H_

#include <exception>
#include <string>
#include <vector>

#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/vault/utils.h"


namespace maidsafe {

namespace vault {

template<typename Data>
void MetadataManager::HandlePutMessage(const nfs::Message& message,
                                       const routing::ReplyFunctor& reply_functor) {
  if (!detail::NodeRangeCheck(routing_, message.source().node_id)) {
    reply_functor(nfs::ReturnCode(-1).Serialise()->string());
    return;
  }

  if (request_queue_.Push(message.id(), message.name())) {
    nfs::OnError on_error_callback = [this] (nfs::Message message) {
                                        this->OnPutErrorHandler<Data>(message);
                                     };
    nfs_.Put<Data>(message, on_error_callback);
  }
  reply_functor(nfs::ReturnCode(0).Serialise()->string());
}

template<typename Data>
void MetadataManager::HandleGetMessage(nfs::Message message,
                                       const routing::ReplyFunctor& reply_functor) {
  std::vector<Identity> online_dataholders(
      data_elements_manager_.GetOnlinePmid(Identity(message.name())));
  std::vector<std::future<Data>> futures;
  for (auto& online_dataholder : online_dataholders)
    futures.emplace_back(nfs_.Get<Data>(message.name(),
                                        nfs::Message::Source(nfs::PersonaType::kMetadataManager,
                                                             routing_.kNodeId()),
                                        online_dataholder));

  auto fetched_data = futures.begin();
  while (futures.size() > 0) {
    if (fetched_data->wait_for(0) == std::future_status::ready) {
      try {
        Data fetched_chunk = fetched_data->get();
        if (fetched_chunk.name().data.IsInitialised()) {
          reply_functor(fetched_chunk.data.string());
          return;
        }
      } catch(...) {
        // no op
      }
      fetched_data = futures.erase(fetched_data);
    } else {
      ++fetched_data;
    }
    Sleep(boost::posix_time::milliseconds(1));
    if (fetched_data == futures.end())
      fetched_data = futures.begin();
  }

  reply_functor(nfs::ReturnCode(-1).Serialise()->string());
}

// On error handler's
template<typename Data>
void MetadataManager::OnPutErrorHandler(nfs::Message message) {
  if (detail::NodeRangeCheck(routing_, message.source().node_id))
    nfs_.Put<Data>(message,
                   [this] (nfs::Message message) {
                     this->OnPutErrorHandler<Data>(message);
                   });
}

template<typename Data>
void MetadataManager::OnDeleteErrorHandler(nfs::Message message) {
  if (detail::NodeRangeCheck(routing_, message.source().node_id))
    nfs_.Delete<Data>(message,
                      [this] (nfs::Message message) {
                        this->OnDeleteErrorHandler<Data>(message);
                      });
}

template<typename Data>
void MetadataManager::OnPostErrorHandler(nfs::PostMessage message) {}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_MANAGER_INL_H_
