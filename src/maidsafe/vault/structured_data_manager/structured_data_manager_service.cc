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

#include "maidsafe/vault/structured_data_manager/structured_data_manager_service.h"

#include <exception>
#include <string>

#include "maidsafe/common/error.h"
#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/common/on_scope_exit.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/nfs/utils.h"
#include "maidsafe/nfs/reply.h"

#include "maidsafe/vault/utils.h"

#include "maidsafe/nfs/pmid_registration.h"
#include "maidsafe/nfs/persona_id.h"
#include "maidsafe/nfs/structured_data.h"
#include "maidsafe/vault/sync.h"
#include "maidsafe/vault/structured_data_manager/structured_data_key.h"
#include "maidsafe/vault/structured_data_manager/structured_data_value.h"
#include "maidsafe/vault/structured_data_manager/structured_data_db.h"

namespace maidsafe {

namespace vault {

namespace {

template<typename Message>
inline bool ForThisPersona(const Message& message) {
  return message.destination_persona() == nfs::Persona::kStructuredDataManager;
}

template<typename Message>
inline bool FromStructuredDataManager(const Message& message) {
  return message.destination_persona() == nfs::Persona::kStructuredDataManager;
}

}  // unnamed namespace

StructuredDataManagerService::StructuredDataManagerService(const passport::Pmid& pmid,
                                                   routing::Routing& routing,
                                                   nfs::PublicKeyGetter& public_key_getter,
                                                   const boost::filesystem::path& path)
    : routing_(routing),
      public_key_getter_(public_key_getter),
      accumulator_mutex_(),
      account_name_(std::make_pair(Identity(routing_.kNodeId().string()),
                                   Identity(pmid.name().data.string()))),
      accumulator_(),
      structured_data_db_(path),
      nfs_(routing_, pmid) {}


void StructuredDataManagerService::ValidateClientSender(const nfs::Message& message) const {
  if (!routing_.IsConnectedClient(message.source().node_id))
    ThrowError(VaultErrors::permission_denied);
  if (!(FromClientMaid(message) || FromClientMpid(message)) || !ForThisPersona(message))
    ThrowError(CommonErrors::invalid_parameter);
}

void StructuredDataManagerService::ValidateSyncSender(const nfs::Message& message) const {
  if (!routing_.IsConnectedVault(message.source().node_id))
    ThrowError(VaultErrors::permission_denied);
  if (!FromStructuredDataManager(message) || !ForThisPersona(message))
    ThrowError(CommonErrors::invalid_parameter);
}

StructuredDataDb::Key StructuredDataManagerService::GetKeyFromMessage(const nfs::Message& message)
                                                                      const {
   if (!message.data().type)
     ThrowError(CommonErrors::parsing_error);
   return std::make_pair(GetDataNameVariant(*message.data().type, message.data().name),
                         message.source());
}

std::vector<StructuredDataVersions::VersionName>
            StructuredDataManagerService::GetVersionsFromMessage(const nfs::Message& msg) const {

   return nfs::StructuredData(nfs::StructuredData::serialised_type(msg.data().content)).Versions();
}



// =============== Get data =================================================================

void StructuredDataManagerService::HandleGet(const nfs::Message& message,
                                             routing::ReplyFunctor reply_functor) {
  try {
    nfs::Reply reply(CommonErrors::success);
    StructuredDataVersions version(structured_data_db_.Get(GetKeyFromMessage(message)));
    reply.data() = nfs::StructuredData(version.Get()).Serialise().data;
    reply_functor(reply.Serialise()->string());
    std::lock_guard<std::mutex> lock(accumulator_mutex_);
    accumulator_.SetHandled(message, maidsafe_error(CommonErrors::success));
  }
  catch (std::exception& e) {
    LOG(kError) << "Bad message: " << e.what();
    nfs::Reply reply(VaultErrors::failed_to_handle_request);
    std::lock_guard<std::mutex> lock(accumulator_mutex_);
    accumulator_.SetHandled(message, maidsafe_error(VaultErrors::failed_to_handle_request));
 }
}

void StructuredDataManagerService::HandleGetBranch(const nfs::Message& message,
                                                   routing::ReplyFunctor reply_functor) {

  try {
    nfs::Reply reply(CommonErrors::success);
    StructuredDataVersions version(structured_data_db_.Get(GetKeyFromMessage(message)));
    auto branch_to_get(GetVersionsFromMessage(message));
    reply.data() = nfs::StructuredData(version.GetBranch(branch_to_get.front())).Serialise().data;
    reply_functor(reply.Serialise()->string());
    std::lock_guard<std::mutex> lock(accumulator_mutex_);
    accumulator_.SetHandled(message, maidsafe_error(CommonErrors::success));
  }
  catch (std::exception& e) {
    LOG(kError) << "Bad message: " << e.what();
    std::lock_guard<std::mutex> lock(accumulator_mutex_);
    accumulator_.SetHandled(message, maidsafe_error(VaultErrors::failed_to_handle_request));
 }
}

void StructuredDataManagerService::AddToAccumulator(const nfs::Message& message) {
  std::lock_guard<std::mutex> lock(accumulator_mutex_);
  accumulator_.SetHandled(message, maidsafe_error(CommonErrors::success));
}


// // =============== Sync ============================================================================

void StructuredDataManagerService::HandleSync(const nfs::Message& /*message*/) {
}

// In this persona we sync all mutating actions, on sucess/fail the reply_functor is fired
// The mergePloicy will supply the reply_functor with the appropriate 'error_code'
template<typename Data>
void StructuredDataManagerService::Sync(const nfs::Message&/* message*/) {
}
// // =============== Account transfer ================================================================
void StructuredDataManagerService::HandleAccountTransfer(const nfs::Message& /*message*/) {
}

}  // namespace vault

}  // namespace maidsafe
