/*  Copyright 2013 MaidSafe.net limited

    This MaidSafe Software is licensed to you under (1) the MaidSafe.net Commercial License,
    version 1.0 or later, or (2) The General Public License (GPL), version 3, depending on which
    licence you accepted on initial access to the Software (the "Licences").

    By contributing code to the MaidSafe Software, or to this project generally, you agree to be
    bound by the terms of the MaidSafe Contributor Agreement, version 1.0, found in the root
    directory of this project at LICENSE, COPYING and CONTRIBUTOR respectively and also
    available at: http://www.novinet.com/license

    Unless required by applicable law or agreed to in writing, the MaidSafe Software distributed
    under the GPL Licence is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
    OF ANY KIND, either express or implied.

    See the Licences for the specific language governing permissions and limitations relating to
    use of the MaidSafe Software.                                                                 */

#include "maidsafe/vault/messages.h"

namespace maidsafe {

namespace vault {

DataNameAndContentAndReturnCode::DataNameAndContentAndReturnCode(
    const DataNameAndContentAndReturnCode& data)
        : name(data.name),
          content(data.content),
          code(data.code) {}

DataNameAndContentAndReturnCode::DataNameAndContentAndReturnCode(
    const DataTagValue& type_in,
    const Identity& name_in,
    const NonEmptyString& content_in,
    const nfs_client::ReturnCode& code_in)
        : name(nfs_vault::DataName(type_in, name_in)),
          content(content_in),
          code(code_in) {}

DataNameAndContentAndReturnCode::DataNameAndContentAndReturnCode()
    : name(), content(), code() {}

DataNameAndContentAndReturnCode::DataNameAndContentAndReturnCode(
    DataNameAndContentAndReturnCode&& other)
        : name(std::move(other.name)),
          content(std::move(other.content)),
          code(std::move(other.code)) {}

DataNameAndContentAndReturnCode& DataNameAndContentAndReturnCode::operator=(
    DataNameAndContentAndReturnCode other) {
  swap(*this, other);
  return *this;
}

void swap(DataNameAndContentAndReturnCode& lhs,
          DataNameAndContentAndReturnCode& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.name, rhs.name);
  swap(lhs.content, rhs.content);
  swap(lhs.code, rhs.code);
}

}  // namespace vault

}  // namespace maidsafe
