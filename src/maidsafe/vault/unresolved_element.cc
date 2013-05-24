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

#include "maidsafe/vault/unresolved_element.h"

#include <string>

#include "maidsafe/common/error.h"

#include "maidsafe/vault/unresolved_element.pb.h"


namespace maidsafe {

namespace vault {

template<>
UnresolvedElement<int32_t, std::pair<DataNameVariant, nfs::MessageAction>>::UnresolvedElement(const serialised_type& serialised_copy)
    : key(),
      messages_contents(),
      sync_counter(0),
      dont_add_to_db(false) {
  protobuf::MaidAndPmidUnresolvedEntry proto_copy;
  if (!proto_copy.ParseFromString(serialised_copy->string()))
    ThrowError(CommonErrors::parsing_error);

  key = std::make_pair(GetDataNameVariant(static_cast<DataTagValue>(proto_copy.key().type()),
                                          Identity(proto_copy.key().name())),
                       static_cast<nfs::MessageAction>(proto_copy.key().action()));
  if (!(key.second == nfs::MessageAction::kPut || key.second == nfs::MessageAction::kDelete))
    ThrowError(CommonErrors::parsing_error);

  // TODO(Fraser#5#): 2013-04-18 - Replace magic number below
  if (proto_copy.messages_contents_size() > 2)
    ThrowError(CommonErrors::parsing_error);

  for (int i(0); i != proto_copy.messages_contents_size(); ++i) {
    MessageContent message_content;
    message_content.peer_id = NodeId(proto_copy.messages_contents(i).peer());
    if (proto_copy.messages_contents(i).has_entry_id())
      message_content.entry_id = proto_copy.messages_contents(i).entry_id();
    if (proto_copy.messages_contents(i).has_value())
      message_content.value = proto_copy.messages_contents(i).value();
    messages_contents.push_back(message_content);
  }

  if (!proto_copy.has_dont_add_to_db())
    ThrowError(CommonErrors::parsing_error);
  dont_add_to_db = proto_copy.dont_add_to_db();
}

template<>
UnresolvedElement<int32_t, std::pair<DataNameVariant, nfs::MessageAction>>::serialised_type
  UnresolvedElement<int32_t, std::pair<DataNameVariant, nfs::MessageAction>>::Serialise() const {
  protobuf::MaidAndPmidUnresolvedEntry proto_copy;
  auto tag_value_and_id(boost::apply_visitor(GetTagValueAndIdentityVisitor(), key.first));

  auto proto_key(proto_copy.mutable_key());
  proto_key->set_type(static_cast<int32_t>(tag_value_and_id.first));
  proto_key->set_name(tag_value_and_id.second.string());
  proto_key->set_action(static_cast<int32_t>(key.second));

  for (const auto& message_content : messages_contents) {
    auto proto_message_content(proto_copy.add_messages_contents());
    proto_message_content->set_peer(message_content.peer_id.string());
    if (message_content.entry_id)
      proto_message_content->set_entry_id(*message_content.entry_id);
    if (message_content.value)
      proto_message_content->set_value(*message_content.value);
  }

  proto_copy.set_dont_add_to_db(dont_add_to_db);

  return serialised_type((NonEmptyString(proto_copy.SerializeAsString())));
}

}  // namespace vault

}  // namespace maidsafe
