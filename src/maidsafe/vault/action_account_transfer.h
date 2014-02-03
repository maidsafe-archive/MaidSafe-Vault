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

#ifndef MAIDSAFE_VAULT_ACTION_ACCOUNT_TRANSFER_H_
#define MAIDSAFE_VAULT_ACTION_ACCOUNT_TRANSFER_H_

#include <cstdint>
#include <string>

#include "boost/optional/optional.hpp"

#include "maidsafe/nfs/types.h"
#include "maidsafe/vault/config.h"

namespace maidsafe {

namespace vault {


// TODO(Prakash) this can be further be optimised by just taking string and directly applying to db

template <typename Value>
class ActionAccountTransfer {
 public:
  //explicit ActionAccountTransfer(Value value);
  explicit ActionAccountTransfer(const std::string& serialised_action);
  ActionAccountTransfer(const ActionAccountTransfer& other);
  ActionAccountTransfer(ActionAccountTransfer&& other);
  std::string Serialise() const;

  detail::DbAction operator()(std::unique_ptr<Value>& value) const;

  static const nfs::MessageAction kActionId = nfs::MessageAction::kAccountTransfer;
  const Value kValue;

 private:
  ActionAccountTransfer();
  ActionAccountTransfer& operator=(ActionAccountTransfer other);
};

template <typename Value>
bool operator==(const ActionAccountTransfer<Value>& lhs,
                const ActionAccountTransfer<Value>& rhs);

template <typename Value>
bool operator!=(const ActionAccountTransfer<Value>& lhs,
                const ActionAccountTransfer<Value>& rhs);


// Implementation

//ActionAccountTransfer::ActionAccountTransfer(Value value) : kValue(value) {}
template <typename Value>
ActionAccountTransfer<Value>::ActionAccountTransfer(const std::string& serialised_action)
    : kValue(serialised_action) {}

template <typename Value>
ActionAccountTransfer<Value>::ActionAccountTransfer(const ActionAccountTransfer& other)
    : kValue(other.kValue) {}

template <typename Value>
ActionAccountTransfer<Value>::ActionAccountTransfer(ActionAccountTransfer&& other)
    : kValue(std::move(other.kValue)) {}

template <typename Value>
std::string ActionAccountTransfer<Value>::Serialise() const {
  return kValue.Serialise();
}

template <typename Value>
detail::DbAction ActionAccountTransfer<Value>::operator()(
        std::unique_ptr<Value>& value) const {
  if (!value)
    value.reset(new Value(kValue));
  return detail::DbAction::kPut;
}

template <typename Value>
bool operator==(const ActionAccountTransfer<Value>& lhs,
                const ActionAccountTransfer<Value>& rhs) {
  return lhs.kValue == rhs.kValue;
}

template <typename Value>
bool operator!=(const ActionAccountTransfer<Value>& lhs,
                const ActionAccountTransfer<Value>& rhs) {
  return !operator==(lhs, rhs);
}






}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_ACTION_ACCOUNT_TRANSFER_H_
