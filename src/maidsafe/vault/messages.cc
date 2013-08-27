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
