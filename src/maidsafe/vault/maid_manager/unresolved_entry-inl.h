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

#ifndef MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_UNRESOLVED_ENTRY_INL_H_
#define MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_UNRESOLVED_ENTRY_INL_H_

#include <algorithm>


namespace maidsafe {

namespace vault {

template<nfs::MessageAction action>
MaidAccountUnresolvedEntry<action>::MaidAccountUnresolvedEntry() : core() {}

MaidAccountUnresolvedEntry<nfs::MessageAction::kPut>::MaidAccountUnresolvedEntry()
    : core(),
      dont_add_to_db(false) {}

template<nfs::MessageAction action>
MaidAccountUnresolvedEntry<action>::MaidAccountUnresolvedEntry(
    const MaidAccountUnresolvedEntry& other)
        : core(other.core) {}

MaidAccountUnresolvedEntry<nfs::MessageAction::kPut>::MaidAccountUnresolvedEntry(
    const MaidAccountUnresolvedEntry& other)
        : core(other.core),
          dont_add_to_db(other.dont_add_to_db) {}

template<nfs::MessageAction action>
MaidAccountUnresolvedEntry<action>::MaidAccountUnresolvedEntry(MaidAccountUnresolvedEntry&& other)
    : core(std::move(other.core)) {}

MaidAccountUnresolvedEntry<nfs::MessageAction::kPut>::MaidAccountUnresolvedEntry(
    MaidAccountUnresolvedEntry&& other)
        : core(std::move(other.core)),
          dont_add_to_db(std::move(other.dont_add_to_db)) {}

template<nfs::MessageAction action>
MaidAccountUnresolvedEntry<action>& MaidAccountUnresolvedEntry<action>::operator=(
    MaidAccountUnresolvedEntry other) {
  swap(*this, other);
  return *this;
}

MaidAccountUnresolvedEntry<nfs::MessageAction::kPut>&
    MaidAccountUnresolvedEntry<nfs::MessageAction::kPut>::operator=(
        MaidAccountUnresolvedEntry other) {
  swap(*this, other);
  return *this;
}


template<nfs::MessageAction action>
MaidAccountUnresolvedEntry<action>::MaidAccountUnresolvedEntry(
    const MaidManager::DbKey& key,
    const ActionAttributes<MaidManager, action>& action_attributes)
        : core(key, action_attributes) {}

MaidAccountUnresolvedEntry<nfs::MessageAction::kPut>::MaidAccountUnresolvedEntry(
    const MaidManager::DbKey& key,
    const ActionAttributes<MaidManager, nfs::MessageAction::kPut>& action_attributes)
        : core(key, action_attributes),
          dont_add_to_db(false) {}

//
//
//
//
//template<nfs::MessageAction action>
//MaidAccountUnresolvedEntry::UnresolvedElement(const nfs::Message& message)
//    : key(),
//      messages_contents(),
//      sync_counter(0),
//      dont_add_to_db(false) {
//  protobuf::MaidAndPmidUnresolvedEntry proto_copy;
//  if (!proto_copy.ParseFromString(serialised_copy->string()))
//    ThrowError(CommonErrors::parsing_error);
//
//  key = std::make_pair(GetDataNameVariant(static_cast<DataTagValue>(proto_copy.key().type()),
//                                          Identity(proto_copy.key().name())),
//                       static_cast<nfs::MessageAction>(proto_copy.key().action()));
//  if (!(key.second == nfs::MessageAction::kPut || key.second == nfs::MessageAction::kDelete))
//    ThrowError(CommonErrors::parsing_error);
//
//  // TODO(Fraser#5#): 2013-04-18 - Replace magic number below
//  if (proto_copy.messages_contents_size() > 2)
//    ThrowError(CommonErrors::parsing_error);
//
//  for (int i(0); i != proto_copy.messages_contents_size(); ++i) {
//    MessageContent message_content;
//    message_content.peer_id = NodeId(proto_copy.messages_contents(i).peer());
//    if (proto_copy.messages_contents(i).has_entry_id())
//      message_content.entry_id = proto_copy.messages_contents(i).entry_id();
//    if (proto_copy.messages_contents(i).has_value())
//      message_content.value = proto_copy.messages_contents(i).value();
//    messages_contents.push_back(message_content);
//  }
//
//  if (!proto_copy.has_dont_add_to_db())
//    ThrowError(CommonErrors::parsing_error);
//  dont_add_to_db = proto_copy.dont_add_to_db();
//}
//
//MaidAccountUnresolvedEntry<nfs::MessageAction::kPut>::UnresolvedElement(const nfs::Message& message)
//    : key(),
//      messages_contents(),
//      sync_counter(0),
//      dont_add_to_db(false) {
//  protobuf::MaidAndPmidUnresolvedEntry proto_copy;
//  if (!proto_copy.ParseFromString(serialised_copy->string()))
//    ThrowError(CommonErrors::parsing_error);
//
//  key = std::make_pair(GetDataNameVariant(static_cast<DataTagValue>(proto_copy.key().type()),
//                                          Identity(proto_copy.key().name())),
//                       static_cast<nfs::MessageAction>(proto_copy.key().action()));
//  if (!(key.second == nfs::MessageAction::kPut || key.second == nfs::MessageAction::kDelete))
//    ThrowError(CommonErrors::parsing_error);
//
//  // TODO(Fraser#5#): 2013-04-18 - Replace magic number below
//  if (proto_copy.messages_contents_size() > 2)
//    ThrowError(CommonErrors::parsing_error);
//
//  for (int i(0); i != proto_copy.messages_contents_size(); ++i) {
//    MessageContent message_content;
//    message_content.peer_id = NodeId(proto_copy.messages_contents(i).peer());
//    if (proto_copy.messages_contents(i).has_entry_id())
//      message_content.entry_id = proto_copy.messages_contents(i).entry_id();
//    if (proto_copy.messages_contents(i).has_value())
//      message_content.value = proto_copy.messages_contents(i).value();
//    messages_contents.push_back(message_content);
//  }
//
//  if (!proto_copy.has_dont_add_to_db())
//    ThrowError(CommonErrors::parsing_error);
//  dont_add_to_db = proto_copy.dont_add_to_db();
//}
//
//template<nfs::MessageAction action>
//MaidAccountUnresolvedEntry::serialised_type MaidAccountUnresolvedEntry::Serialise() const {
//  protobuf::MaidAndPmidUnresolvedEntry proto_copy;
//  auto tag_value_and_id(boost::apply_visitor(GetTagValueAndIdentityVisitor(), key.first));
//
//  auto proto_key(proto_copy.mutable_key());
//  proto_key->set_type(static_cast<uint32_t>(tag_value_and_id.first));
//  proto_key->set_name(tag_value_and_id.second.string());
//  proto_key->set_action(static_cast<int32_t>(key.second));
//
//  for (const auto& message_content : messages_contents) {
//    auto proto_message_content(proto_copy.add_messages_contents());
//    proto_message_content->set_peer(message_content.peer_id.string());
//    if (message_content.entry_id)
//      proto_message_content->set_entry_id(*message_content.entry_id);
//    if (message_content.value)
//      proto_message_content->set_value(*message_content.value);
//  }
//
//  proto_copy.set_dont_add_to_db(dont_add_to_db);
//
//  return serialised_type((NonEmptyString(proto_copy.SerializeAsString())));
//}
//
//MaidAccountUnresolvedEntry<nfs::MessageAction::kPut>::serialised_type
//    MaidAccountUnresolvedEntry<nfs::MessageAction::kPut>::Serialise() const {
//  protobuf::MaidAndPmidUnresolvedEntry proto_copy;
//  auto tag_value_and_id(boost::apply_visitor(GetTagValueAndIdentityVisitor(), key.first));
//
//  auto proto_key(proto_copy.mutable_key());
//  proto_key->set_type(static_cast<uint32_t>(tag_value_and_id.first));
//  proto_key->set_name(tag_value_and_id.second.string());
//  proto_key->set_action(static_cast<int32_t>(key.second));
//
//  for (const auto& message_content : messages_contents) {
//    auto proto_message_content(proto_copy.add_messages_contents());
//    proto_message_content->set_peer(message_content.peer_id.string());
//    if (message_content.entry_id)
//      proto_message_content->set_entry_id(*message_content.entry_id);
//    if (message_content.value)
//      proto_message_content->set_value(*message_content.value);
//  }
//
//  proto_copy.set_dont_add_to_db(dont_add_to_db);
//
//  return serialised_type((NonEmptyString(proto_copy.SerializeAsString())));
//}

template<nfs::MessageAction action>
void swap(MaidAccountUnresolvedEntry<action>& lhs,
          MaidAccountUnresolvedEntry<action>& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.core, rhs.core);
  swap(lhs.dont_add_to_db, rhs.dont_add_to_db);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_ACCOUNT_HOLDER_MAID_ACCOUNT_UNRESOLVED_ENTRY_INL_H_
