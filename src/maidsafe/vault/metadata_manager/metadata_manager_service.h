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

#include <memory>
#include <mutex>
#include <vector>

#include "boost/filesystem/path.hpp"

#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/routing/api_config.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/message.h"
#include "maidsafe/nfs/nfs.h"
#include "maidsafe/nfs/public_key_getter.h"

#include "maidsafe/vault/accumulator.h"
#include "maidsafe/vault/metadata_manager/metadata_helpers.h"
#include "maidsafe/vault/metadata_manager/metadata_handler.h"
#include "maidsafe/vault/metadata_manager/metadata.pb.h"
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
  void HandleMessage(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  void HandleMessage(const nfs::Message& /*message*/, const routing::ReplyFunctor& /*reply_functor*/) {}
  void HandleChurnEvent(routing::MatrixChange matrix_change);

 private:
  template<typename Data>
  struct GetHandler {
    GetHandler(const routing::ReplyFunctor& reply_functor_in,
               size_t holder_count_in,
               const nfs::MessageId& message_id_in);
    routing::ReplyFunctor reply_functor;
    size_t holder_count;
    nfs::MessageId message_id;
    std::mutex mutex;
    crypto::SHA512Hash validation_result;
    std::vector<protobuf::DataOrProof> data_holder_results;

   private:
    GetHandler(const GetHandler&);
    GetHandler& operator=(const GetHandler&);
    GetHandler(GetHandler&&);
    GetHandler& operator=(GetHandler&&);
  };

  MetadataManagerService(const MetadataManagerService&);
  MetadataManagerService& operator=(const MetadataManagerService&);
  MetadataManagerService(MetadataManagerService&&);
  MetadataManagerService& operator=(MetadataManagerService&&);

  template<typename Data>
  void HandlePut(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void Put(const Data& data, const PmidName& target_data_holder);

  template<typename Data>
  void HandleGet(nfs::Message message, const routing::ReplyFunctor& reply_functor);
  template<typename Data>
  void OnHandleGet(std::shared_ptr<GetHandler<Data>> get_handler,
                   const std::string& serialised_reply);
  template<typename Data>
  void IntegrityCheck(std::shared_ptr<GetHandler<Data>> get_handler);
  template<typename Data>
  void HandleDelete(const nfs::Message& message, const routing::ReplyFunctor& reply_functor);

  void ValidatePutSender(const nfs::Message& message) const;
  void ValidateGetSender(const nfs::Message& message) const;
  void ValidateDeleteSender(const nfs::Message& message) const;
  void ValidatePostSender(const nfs::Message& message) const;

  void HandleNodeDown(const nfs::Message& message);
  void HandleNodeUp(const nfs::Message& message);

  template<typename Data>
  void HandlePutResult(const nfs::Reply& overall_result);
  template<typename Data>
  void HandleGetReply(std::string serialised_reply);

  template<typename Data>
  void OnGenericErrorHandler(nfs::Message message);

  bool ThisVaultInGroupForData(const nfs::Message& message) const;

  template<typename Data, nfs::MessageAction action>
  void AddLocalUnresolvedEntryThenSync(const nfs::Message& message,
                                       const MetadataValueDelta& delta);

  // =============== Sync ==========================================================================
  void Sync(const DataNameVariant& record_name);

  void HandleSync(const nfs::Message& message);

  // =============== Account transfer ==============================================================
  void TransferAccount(const DataNameVariant& record_name, const NodeId& new_node);
  void HandleAccountTransfer(const nfs::Message& message);

  routing::Routing& routing_;
  nfs::PublicKeyGetter& public_key_getter_;
  std::mutex accumulator_mutex_;
  Accumulator<DataNameVariant> accumulator_;
  MetadataHandler metadata_handler_;
  MetadataManagerNfs nfs_;
  static const int kPutRequestsRequired_;
  static const int kPutRepliesSuccessesRequired_;
  static const int kDeleteRequestsRequired_;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/metadata_manager/metadata_manager_service-inl.h"

#endif  // MAIDSAFE_VAULT_METADATA_MANAGER_METADATA_MANAGER_SERVICE_H_
