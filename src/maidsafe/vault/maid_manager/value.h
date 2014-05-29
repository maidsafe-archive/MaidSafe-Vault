/*  Copyright 2012 MaidSafe.net limited

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

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_VALUE_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_VALUE_H_

#include "maidsafe/common/config.h"

#include <cstdint>
#include <string>

namespace maidsafe {

namespace vault {

class MaidManagerValue {
 public:
  explicit MaidManagerValue(const std::string& serialised_maid_manager_value);
  MaidManagerValue();
  MaidManagerValue(MaidManagerValue&& other) MAIDSAFE_NOEXCEPT;
  MaidManagerValue& operator=(MaidManagerValue other);
  std::string Serialise() const;

  void Put(int32_t cost);
  void IncrementCount();
  void DecrementCount();
  // Returns amount which was subtracted from 'total_cost'.
  int32_t Delete();
  int32_t count() const { return count_; }
  int64_t total_cost() const { return total_cost_; }

  friend void swap(MaidManagerValue& lhs, MaidManagerValue& rhs);

 private:
  MaidManagerValue(const MaidManagerValue&);

 private:
  int32_t count_;
  int64_t total_cost_;
};

bool operator==(const MaidManagerValue& lhs, const MaidManagerValue& rhs);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_VALUE_H_
