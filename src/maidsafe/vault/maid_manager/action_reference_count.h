/*  Copyright 2013 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.maidsafe.net/licenses

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_ACTION_REFERENCE_COUNT_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_ACTION_REFERENCE_COUNT_H_

#include <string>

#include "maidsafe/nfs/types.h"
#include "maidsafe/nfs/vault/messages.h"

#include "maidsafe/vault/config.h"

namespace maidsafe {

namespace vault {

template <bool Increment>
struct ActionMaidManagerReferenceCount {  
  detail::DbAction operator()(MaidManagerMetadata& metadata,
                              std::unique_ptr<MaidManagerValue>& value) const;
};

template <bool Increment>
detail::DbAction ActionMaidManagerReferenceCount::operator()(
    std::unique_ptr<MaidManagerValue>& value) const {
  if (!value)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::no_such_element));
  value->IncremenetCount();
  return detail::DbAction::kPut;
}

template <>
detail::DbAction ActionMaidManagerReferenceCount<false>::operator()(
    std::unique_ptr<MaidManagerValue>& value) const {
  if (!value)
    BOOST_THROW_EXCEPTION(MakeError(CommonErrors::no_such_element));
  assert((value->count() > 0));
  value->DecrementCount();
  return detail::DbAction::kPut;
}

typedef ActionMaidManagerReferenceCount<true> ActionMaidManagerIncrementReferenceCount;
typedef ActionMaidManagerReferenceCount<false> ActionMaidManagerDecrementReferenceCount;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_ACTION_REFERENCE_COUNT_H_
