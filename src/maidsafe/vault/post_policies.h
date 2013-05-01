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

#ifndef MAIDSAFE_VAULT_POST_POLICIES_H_
#define MAIDSAFE_VAULT_POST_POLICIES_H_

#include <string>
#include <vector>

#include "maidsafe/common/crypto.h"
#include "maidsafe/common/rsa.h"
#include "maidsafe/common/types.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/routing/api_config.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/message.h"
#include "maidsafe/nfs/generic_message.h"
#include "maidsafe/nfs/types.h"


namespace maidsafe {

namespace vault {

template<typename SyncPolicy, typename PersonaMiscellaneousPolicy>
class VaultPostPolicy : public SyncPolicy, public PersonaMiscellaneousPolicy {
 public:
  VaultPostPolicy(routing::Routing& routing, const passport::Pmid& pmid)
      : SyncPolicy(routing, pmid),
        PersonaMiscellaneousPolicy(routing, pmid) {}
};

template<nfs::Persona source_persona>
class SyncPolicy {
 public:
  SyncPolicy(routing::Routing& routing, const passport::Pmid& pmid)
      : routing_(routing),
        kSource_(source_persona, routing_.kNodeId()),
        kPmid_(pmid) {}

  void TransferAccount(const NodeId& target_node_id, const NonEmptyString& serialised_account) {
    nfs::GenericMessage generic_message(
        nfs::GenericMessage::Action::kAccountTransfer,
        source_persona,
        kSource_,
        Identity(target_node_id.string()),
        serialised_account);
    nfs::Message message(nfs::GenericMessage::message_type_identifier,
                         generic_message.Serialise().data);
    routing_.SendDirect(target_node_id, message.Serialise()->string(), false, nullptr);
  }

  template<typename Name>
  void Sync(const Name& name, const NonEmptyString& serialised_sync_data) {
    nfs::GenericMessage generic_message(
        nfs::GenericMessage::Action::kSynchronise,
        source_persona,
        kSource_,
        name.data,
        serialised_sync_data);

    nfs::Message message(nfs::GenericMessage::message_type_identifier,
                         generic_message.Serialise().data);
    routing_.SendGroup(NodeId(generic_message.name().string()), message.Serialise()->string(),
                       false, nullptr);
  }

 private:
  routing::Routing& routing_;
  const nfs::PersonaId kSource_;
  const passport::Pmid kPmid_;
};

class MaidAccountHolderMiscellaneousPolicy {
 public:
  MaidAccountHolderMiscellaneousPolicy(routing::Routing& routing, const passport::Pmid& pmid)
      : routing_(routing),
        kSource_(nfs::Persona::kMaidAccountHolder, routing_.kNodeId()),
        kPmid_(pmid) {}

  void RequestPmidTotals(const passport::PublicPmid::name_type& pmid_name,
                         const routing::ResponseFunctor& callback) {
    nfs::GenericMessage generic_message(nfs::GenericMessage::Action::kGetPmidTotals,
        nfs::Persona::kPmidAccountHolder, kSource_, pmid_name.data, NonEmptyString());
    nfs::Message message(nfs::GenericMessage::message_type_identifier,
                         generic_message.Serialise().data);
    routing_.SendGroup(NodeId(generic_message.name().string()), message.Serialise()->string(),
                       false, callback);
  }

 private:
  routing::Routing& routing_;
  const nfs::PersonaId kSource_;
  const passport::Pmid kPmid_;
};

class MetadataManagerMiscellaneousPolicy {
 public:
  MetadataManagerMiscellaneousPolicy(routing::Routing& routing, const passport::Pmid& pmid)
      : routing_(routing),
        kSource_(nfs::Persona::kMaidAccountHolder, routing_.kNodeId()),
        kPmid_(pmid) {}

 private:
  routing::Routing& routing_;
  const nfs::PersonaId kSource_;
  const passport::Pmid kPmid_;
};

class PmidAccountHolderMiscellaneousPolicy {
 public:
  PmidAccountHolderMiscellaneousPolicy(routing::Routing& routing, const passport::Pmid& pmid)
      : routing_(routing),
        kSource_(nfs::Persona::kMaidAccountHolder, routing_.kNodeId()),
        kPmid_(pmid) {}

 private:
  routing::Routing& routing_;
  const nfs::PersonaId kSource_;
  const passport::Pmid kPmid_;
};

class DataHolderMiscellaneousPolicy {
 public:
  DataHolderMiscellaneousPolicy(routing::Routing& routing, const passport::Pmid& pmid)
      : routing_(routing),
        kSource_(nfs::Persona::kMaidAccountHolder, routing_.kNodeId()),
        kPmid_(pmid) {}

 private:
  routing::Routing& routing_;
  const nfs::PersonaId kSource_;
  const passport::Pmid kPmid_;
};

typedef VaultPostPolicy<SyncPolicy<nfs::Persona::kMaidAccountHolder>,
                        MaidAccountHolderMiscellaneousPolicy> MaidAccountHolderPostPolicy;

typedef VaultPostPolicy<SyncPolicy<nfs::Persona::kMetadataManager>,
                        MetadataManagerMiscellaneousPolicy> MetadataManagerPostPolicy;

typedef VaultPostPolicy<SyncPolicy<nfs::Persona::kPmidAccountHolder>,
                        PmidAccountHolderMiscellaneousPolicy> PmidAccountHolderPostPolicy;

typedef VaultPostPolicy<SyncPolicy<nfs::Persona::kDataHolder>,
                        DataHolderMiscellaneousPolicy> DataHolderPostPolicy;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_POST_POLICIES_H_
