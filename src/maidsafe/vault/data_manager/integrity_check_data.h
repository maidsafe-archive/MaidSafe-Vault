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

#ifndef MAIDSAFE_VAULT_DATA_MANAGER_INTEGRITY_CHECK_DATA_H_
#define MAIDSAFE_VAULT_DATA_MANAGER_INTEGRITY_CHECK_DATA_H_

#include <cstdint>
#include <string>

#include "maidsafe/common/crypto.h"
#include "maidsafe/common/config.h"
#include "maidsafe/common/types.h"

namespace maidsafe {

namespace vault {

class IntegrityCheckData {
 public:
  typedef crypto::SHA512Hash Result;

  IntegrityCheckData();
  explicit IntegrityCheckData(std::string random_input);
  IntegrityCheckData(std::string random_input, const NonEmptyString& serialised_value);
  IntegrityCheckData(const IntegrityCheckData& other);
  IntegrityCheckData(IntegrityCheckData&& other);
  IntegrityCheckData& operator=(IntegrityCheckData other);

  // Throws if 'random_input_' is empty or if 'result_' is not empty.
  void SetResult(const Result& result);
  // Throws if 'random_input_' or 'result_' is empty.
  bool Validate(const NonEmptyString& serialised_value) const;

  std::string random_input() const { return random_input_; }
  Result result() const { return result_; }

  static std::string GetRandomInput(uint32_t min_size = 64, uint32_t max_size = 128);

  friend void swap(IntegrityCheckData& lhs, IntegrityCheckData& rhs) MAIDSAFE_NOEXCEPT;

 private:
  std::string random_input_;
  Result result_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_DATA_MANAGER_INTEGRITY_CHECK_DATA_H_
