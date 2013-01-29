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

#include "maidsafe/vault/pmid_account_holder_service.h"

#include "maidsafe/common/error.h"

#include "maidsafe/vault/pmid_account_pb.h"

namespace maidsafe {

namespace vault {

PmidAccountHolder::PmidAccountHolder(routing::Routing& routing,
                                     nfs::PublicKeyGetter& public_key_getter,
                                     const boost::filesystem::path vault_root_dir)
  : routing_(routing),
    public_key_getter_(public_key_getter),
    accumulator_(),
    pmid_account_handler_(vault_root_dir),
    nfs_(routing) {}


void PmidAccountHolder::CloseNodeReplaced(const std::vector<routing::NodeInfo>& /*new_close_nodes*/) {  //  NOLINT (fine when not commented)
}

void PmidAccountHolder::ValidateDataMessage(const nfs::DataMessage& data_message) {
  if (!data_message.HasTargetId()) {
    LOG(kError) << "No target ID, can't forward the message.";
    ThrowError(VaultErrors::permission_denied);
  }

  if (routing_.EstimateInGroup(data_message.this_persona().node_id,
                               NodeId(data_message.data().name))) {
    LOG(kError) << "Message doesn't seem to come from the right group.";
    ThrowError(VaultErrors::permission_denied);
  }
}

void PmidAccountHolder::InformOfDataHolderDown(const PmidName& pmid_name) {
  InformAboutDataHolderUp(pmid_name, nfs::GenericMessage::kNodeDown);
}

void PmidAccountHolder::InformOfDataHolderUp(const PmidName& pmid_name) {
  InformAboutDataHolderUp(pmid_name, nfs::GenericMessage::kNodeUp);
}

void PmidAccountHolder::InformAboutDataHolder(const PmidName& pmid_name,
                                              nfs::GenericMessage::Action action) {
  std::vector<PmidName> metada_manager_ids;
  GetDataNamesInAccount(pmid_name, metada_manager_ids);
  for (PmidName& mm_id : metada_manager_ids)
    SendSyncMessage(mm_id, action);
}

void PmidAccountHolder::GetDataNamesInAccount(const PmidName& pmid_name,
                                              std::vector<PmidName>& metada_manager_ids) {
  // TODO(Team): This function could be simplified if the account handler could give all
  //             data names for a particular account
  NonEmptyString serialsied_account(pmid_account_handler_.GetSerialisedAccount(pmid_name));
  protobuf::PmidAccount account;
  account.ParseFromString(serialsied_account.string());

  for (int n(0); n != account.recent_data_stored_size(); ++n)
    metada_manager_ids.push_back(PmidName(account.recent_data_stored(n).name()));

}

void PmidAccountHolder::SendSyncMessage(const PmidName& pmid_name,
                                        nfs::GenericMessage::Action action) {
  nfs::GenericMessage generic_message(action,
                                      nfs::Persona::kDataHolder,
                                      nfs::PersonaId(nfs::Persona::kPmidAccountHolder,
                                                     routing_.kNodeId()),
                                      pmid_name,
                                      NonEmptyString());
  // nullptr as on_error functor since it's not used in the post policy
  nfs_.PostSyncData<nfs::Persona::kPmidAccountHolder>(generic_message, nullptr);
}

}  // namespace vault

}  // namespace maidsafe
