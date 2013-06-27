/* Copyright 2013 MaidSafe.net limited

This MaidSafe Software is licensed under the MaidSafe.net Commercial License, version 1.0 or later,
and The General Public License (GPL), version 3. By contributing code to this project You agree to
the terms laid out in the MaidSafe Contributor Agreement, version 1.0, found in the root directory
of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also available at:

http://www.novinet.com/license

Unless required by applicable law or agreed to in writing, software distributed under the License is
distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
implied. See the License for the specific language governing permissions and limitations under the
License.
*/

#ifndef MAIDSAFE_VAULT_UNRESOLVED_ELEMENT_INL_H_
#define MAIDSAFE_VAULT_UNRESOLVED_ELEMENT_INL_H_

#include <algorithm>


namespace maidsafe {

namespace vault {

template<typename PersonaTypes>
UnresolvedElement<PersonaTypes>::UnresolvedElement()
    : key(),
      messages_contents(),
      sync_counter(0),
      dont_add_to_db(false) {}

template<typename PersonaTypes>
UnresolvedElement<PersonaTypes>::UnresolvedElement(const UnresolvedElement& other)
    : key(other.key),
      messages_contents(other.messages_contents),
      sync_counter(other.sync_counter),
      dont_add_to_db(other.dont_add_to_db) {}

template<typename PersonaTypes>
UnresolvedElement<PersonaTypes>::UnresolvedElement(UnresolvedElement&& other)
    : key(std::move(other.key)),
      messages_contents(std::move(other.messages_contents)),
      sync_counter(std::move(other.sync_counter)),
      dont_add_to_db(std::move(other.dont_add_to_db)) {}

template<typename PersonaTypes>
UnresolvedElement<PersonaTypes>& UnresolvedElement<PersonaTypes>::operator=(
    UnresolvedElement other) {
  swap(*this, other);
  return *this;
}

template<typename PersonaTypes>
UnresolvedElement<PersonaTypes>::UnresolvedElement(const Key& key,
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

template<typename PersonaTypes>
void swap(UnresolvedElement<PersonaTypes>& lhs,
          UnresolvedElement<PersonaTypes>& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.key, rhs.key);
  swap(lhs.messages_contents, rhs.messages_contents);
  swap(lhs.sync_counter, rhs.sync_counter);
  swap(lhs.dont_add_to_db, rhs.dont_add_to_db);
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_UNRESOLVED_ELEMENT_INL_H_
