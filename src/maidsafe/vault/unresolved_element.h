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

#ifndef MAIDSAFE_VAULT_UNRESOLVED_ELEMENT_H_
#define MAIDSAFE_VAULT_UNRESOLVED_ELEMENT_H_

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

#include "boost/optional/optional.hpp"

#include "maidsafe/common/node_id.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/nfs/types.h"


namespace maidsafe {

namespace vault {

template<typename Persona, typename Action>
struct UnresolvedElement {

 private:
  template<typename Persona, typename Action>
  struct SerialisedUnresolvedEntryTag {};

 public:
  typedef TaggedValue<NonEmptyString, SerialisedUnresolvedEntryTag<Persona, Action>>
                                                                               serialised_type;

  UnresolvedElement();
  explicit UnresolvedElement(const serialised_type& serialised_copy);
  UnresolvedElement(const UnresolvedElement& other);
  UnresolvedElement(UnresolvedElement&& other);
  UnresolvedElement& operator=(UnresolvedElement other);
  UnresolvedElement(typename const Persona::Key& key,
                    const Action& action,
                    const NodeId& sender_id);

  template<typename T>
  friend void swap(UnresolvedElement& lhs, UnresolvedElement& rhs) MAIDSAFE_NOEXCEPT;

  serialised_type Serialise() const;

  typename Persona::Key key;
  std::set<NodeId> peer_id;
  int32_t entry_id;
  Action action;
  int sync_counter;
  NodeId source_node_id;
};

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/unresolved_element-inl.h"

#endif  // MAIDSAFE_VAULT_UNRESOLVED_ELEMENT_H_
