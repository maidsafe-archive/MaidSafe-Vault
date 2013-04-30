/***************************************************************************************************
 *  Copyright 2013 MaidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of MaidSafe.net.                                  *
 **************************************************************************************************/

#ifndef MAIDSAFE_VAULT_UNRESOLVED_ENTRY_INL_H_
#define MAIDSAFE_VAULT_UNRESOLVED_ENTRY_INL_H_

#include "maidsafe/vault/unresolved_entry.h"

#include "maidsafe/common/error.h"

#include "maidsafe/vault/unresolved_entry.pb.h"


namespace maidsafe {

namespace vault {

MaidAndPmidUnresolvedEntry::MaidAndPmidUnresolvedEntry()
    : key(),
      peers_and_values(),
      sync_counter(0),
      dont_add_to_db(false) {}

MaidAndPmidUnresolvedEntry::MaidAndPmidUnresolvedEntry(const serialised_type& serialised_copy)
    : key(),
      peers_and_values(),
      sync_counter(0),
      dont_add_to_db(false) {
  protobuf::MaidAndPmidUnresolvedEntry proto_copy;
  if (!proto_copy.ParseFromString(serialised_copy->string()))
    ThrowError(CommonErrors::parsing_error);

  key.data_name = GetDataNameVariant(static_cast<DataTagValue>(proto_copy.type()),
                                     Identity(proto_copy.name()));
  key.action = static_cast<nfs::MessageAction>(proto_copy.action());
  if (proto_copy.has_entry_id())
    key.entry_id = proto_copy.entry_id();
  if (!(key.action == nfs::MessageAction::kPut || key.action == nfs::MessageAction::kDelete))
    ThrowError(CommonErrors::parsing_error);

  // TODO(Fraser#5#): 2013-04-18 - Replace magic number below
  if (proto_copy.values_size() > 2)
    ThrowError(CommonErrors::parsing_error);

  for (int i(0); i != proto_copy.values_size(); ++i) {
    peers_and_values.emplace_back(std::make_pair(NodeId(proto_copy.values(i).peer()),
                                                 proto_copy.values(i).cost()));
  }

  dont_add_to_db = proto_copy.dont_add_to_db();
}

MaidAndPmidUnresolvedEntry::MaidAndPmidUnresolvedEntry(const MaidAndPmidUnresolvedEntry& other)
    : key(other.key),
      peers_and_values(other.peers_and_values),
      sync_counter(other.sync_counter),
      dont_add_to_db(other.dont_add_to_db) {}

MaidAndPmidUnresolvedEntry::MaidAndPmidUnresolvedEntry(MaidAndPmidUnresolvedEntry&& other)
    : key(std::move(other.key)),
      peers_and_values(std::move(other.peers_and_values)),
      sync_counter(std::move(other.sync_counter)),
      dont_add_to_db(std::move(other.dont_add_to_db)) {}

MaidAndPmidUnresolvedEntry& MaidAndPmidUnresolvedEntry::operator=(
    MaidAndPmidUnresolvedEntry other) {
  swap(*this, other);
  return *this;
}

MaidAndPmidUnresolvedEntry::MaidAndPmidUnresolvedEntry(const Key& data_name_and_action,
                                                       Value cost,
                                                       const NodeId& sender_id)
    : key(),
      peers_and_values(),
      sync_counter(0),
      dont_add_to_db(false) {
  key.data_name = 
}

void swap(MaidAndPmidUnresolvedEntry& lhs, MaidAndPmidUnresolvedEntry& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.data_name_and_action, rhs.data_name_and_action);
  swap(lhs.cost, rhs.cost);
  swap(lhs.peers, rhs.peers);
  swap(lhs.sync_counter, rhs.sync_counter);
  swap(lhs.dont_add_to_db, rhs.dont_add_to_db);
}

MaidAndPmidUnresolvedEntry::serialised_type MaidAndPmidUnresolvedEntry::Serialise() const {
  protobuf::MaidAndPmidUnresolvedEntry proto_copy;
  auto tag_value_and_id(boost::apply_visitor(GetTagValueAndIdentityVisitor(),
                                             data_name_and_action.first));

  proto_copy.set_type(static_cast<int32_t>(tag_value_and_id.first));
  proto_copy.set_name(tag_value_and_id.second.string());
  proto_copy.set_action(static_cast<int32_t>(data_name_and_action.second));
  proto_copy.set_cost(cost);
//  for (const auto& peer : peers)
//    *proto_copy.add_peers() = peer.string();
  proto_copy.set_dont_add_to_db(dont_add_to_db);
  return serialised_type((NonEmptyString(proto_copy.SerializeAsString())));
}


}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_UNRESOLVED_ENTRY_INL_H_
