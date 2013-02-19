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

#ifndef MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_MANAGER_SERVICE_H_
#define MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_MANAGER_SERVICE_H_

#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/routing/api_config.h"
#include "maidsafe/nfs/generic_message.h"
#include "maidsafe/nfs/message.h"
#include "maidsafe/nfs/nfs.h"
#include "maidsafe/nfs/public_key_getter.h"

#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/metadata_manager/metadata_handler.h"
#include "maidsafe/vault/types.h"


namespace maidsafe {

namespace vault {

class MetadataManagerService {
 public:
  MetadataManagerService(const passport::Pmid& pmid,
                         routing::Routing& routing,
                         nfs::PublicKeyGetter& public_key_getter,
                         const boost::filesystem::path& vault_root_dir);
  template<typename Data>
  void HandleDataMessage(const nfs::DataMessage& data_message,
                         const routing::ReplyFunctor& reply_functor);
  void HandleGenericMessage(const nfs::GenericMessage& generic_message,
                            const routing::ReplyFunctor& reply_functor);
  void TriggerSync();

 private:
  template<typename Data>
  void HandleGet(nfs::DataMessage data_message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void HandlePut(const nfs::DataMessage& data_message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void HandleDelete(const nfs::DataMessage& data_message,
                    const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void ValidateDataMessage(const nfs::DataMessage& data_message) const;

  void HandleNodeDown(const nfs::GenericMessage& generic_message);
  void HandleNodeUp(const nfs::GenericMessage& generic_message);

  // On error handler
  template<typename Data>
  void OnPutErrorHandler(nfs::DataMessage data_message);
  template<typename Data>
  void OnDeleteErrorHandler(nfs::DataMessage data_message);
  template<typename Data>
  void OnGenericErrorHandler(nfs::GenericMessage message);

  routing::Routing& routing_;
  nfs::PublicKeyGetter& public_key_getter_;
  Accumulator<DataNameVariant> accumulator_;
  MetadataHandler metadata_handler_;
  MetadataManagerNfs nfs_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/metadata_manager/metadata_manager_service-inl.h"

#endif  // MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_MANAGER_SERVICE_H_
