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

#include "maidsafe/vault/integrity_check_data.h"

#include "maidsafe/common/error.h"
#include "maidsafe/common/log.h"
#include "maidsafe/common/utils.h"

namespace maidsafe {

namespace vault {

namespace {

IntegrityCheckData::Result GetResult(const NonEmptyString& serialised_value,
                                     const std::string& random_input) {
  return crypto::Hash<crypto::SHA512>(serialised_value.string() + random_input);
}

}  // unnamed namespace

IntegrityCheckData::IntegrityCheckData() : random_input_(), result_() {}

IntegrityCheckData::IntegrityCheckData(std::string random_input)
    : random_input_(std::move(random_input)), result_() {}

IntegrityCheckData::IntegrityCheckData(std::string random_input,
                                       const NonEmptyString& serialised_value)
    : random_input_(std::move(random_input)),
      result_(GetResult(serialised_value, random_input_)) {}

IntegrityCheckData::IntegrityCheckData(const IntegrityCheckData& other)
    : random_input_(other.random_input_), result_(other.result_) {}

IntegrityCheckData::IntegrityCheckData(IntegrityCheckData&& other)
    : random_input_(std::move(other.random_input_)), result_(std::move(other.result_)) {}

IntegrityCheckData& IntegrityCheckData::operator=(IntegrityCheckData other) {
  swap(*this, other);
  return *this;
}

void IntegrityCheckData::SetResult(const Result& result) {
  if (random_input_.empty() || result_.IsInitialised()) {
    LOG(kError) << "SetResult requires random_input_.empty() && !result_->IsInitialised()";
    ThrowError(CommonErrors::uninitialised);
  }
  result_ = result;
}

bool IntegrityCheckData::Validate(const NonEmptyString& serialised_value) const {
  if (random_input_.empty() || !result_.IsInitialised()) {
    LOG(kError) << "Validate requires !random_input_.empty() && result_->IsInitialised()";
    ThrowError(CommonErrors::uninitialised);
  }
  return GetResult(serialised_value, random_input_) == result_;
}

std::string IntegrityCheckData::GetRandomInput(uint32_t min_size, uint32_t max_size) {
  return RandomString((RandomUint32() % (max_size - min_size)) + min_size);
}

void swap(IntegrityCheckData& lhs, IntegrityCheckData& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.random_input_, rhs.random_input_);
  swap(lhs.result_, rhs.result_);
}

}  // namespace vault

}  // namespace maidsafe
