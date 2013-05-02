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

#ifndef MAIDSAFE_VAULT_UNRESOLVED_ELEMENT_INL_H_
#define MAIDSAFE_VAULT_UNRESOLVED_ELEMENT_INL_H_

#include <algorithm>


namespace maidsafe {

namespace vault {

template<typename ValueType>
UnresolvedElement<ValueType>::UnresolvedElement()
    : key(),
      messages_contents(),
      sync_counter(0),
      dont_add_to_db(false) {}

template<>
UnresolvedElement<int32_t>::UnresolvedElement(const serialised_type& serialised_copy);

template<typename ValueType>
UnresolvedElement<ValueType>::UnresolvedElement(const UnresolvedElement& other)
    : key(other.key),
      messages_contents(other.messages_contents),
      sync_counter(other.sync_counter),
      dont_add_to_db(other.dont_add_to_db) {}

template<typename ValueType>
UnresolvedElement<ValueType>::UnresolvedElement(UnresolvedElement&& other)
    : key(std::move(other.key)),
      messages_contents(std::move(other.messages_contents)),
      sync_counter(std::move(other.sync_counter)),
      dont_add_to_db(std::move(other.dont_add_to_db)) {}

template<typename ValueType>
UnresolvedElement<ValueType>& UnresolvedElement<ValueType>::operator=(UnresolvedElement other) {
  swap(*this, other);
  return *this;
}

template<typename ValueType>
UnresolvedElement<ValueType>::UnresolvedElement(const Key& key,
                                                const Value& value,
                                                const NodeId& sender_id)
    : key(key),
      messages_contents(),
      sync_counter(0),
      dont_add_to_db(false) {
  MessageContent message_content;
  message_content.peer_id = sender_id;
  message_content.value = value;
  messages_contents.push_back(message_content);
}

template<typename ValueType>
void swap(UnresolvedElement<ValueType>& lhs, UnresolvedElement<ValueType>& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.key, rhs.key);
  swap(lhs.messages_contents, rhs.messages_contents);
  swap(lhs.sync_counter, rhs.sync_counter);
  swap(lhs.dont_add_to_db, rhs.dont_add_to_db);
}

template<>
UnresolvedElement<int32_t>::serialised_type UnresolvedElement<int32_t>::Serialise() const;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_UNRESOLVED_ELEMENT_INL_H_
