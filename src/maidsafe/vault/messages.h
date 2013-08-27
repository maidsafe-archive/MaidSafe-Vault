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

#ifndef MAIDSAFE_VAULT_MESSAGE_H_
#define MAIDSAFE_VAULT_MESSAGE_H_

#include "maidsafe/data_types/data_type_values.h"

#include "maidsafe/nfs/client/messages.h"
#include "maidsafe/nfs/vault/messages.h"

namespace maidsafe {

namespace vault {

struct DataNameAndContentAndReturnCode {
  DataNameAndContentAndReturnCode(const DataTagValue& type_in,
                                  const Identity& name_in,
                                  const NonEmptyString& content_in,
                                  const nfs_client::ReturnCode& code_in);
  DataNameAndContentAndReturnCode();
  DataNameAndContentAndReturnCode(const DataNameAndContentAndReturnCode& other);
  DataNameAndContentAndReturnCode(DataNameAndContentAndReturnCode&& other);
  DataNameAndContentAndReturnCode& operator=(DataNameAndContentAndReturnCode other);

  explicit DataNameAndContentAndReturnCode(const std::string& serialised_copy);
  std::string Serialise() const;

  nfs_vault::DataName name;
  NonEmptyString content;
  nfs_client::ReturnCode code;
};

void swap(DataNameAndContentAndReturnCode& lhs,
          DataNameAndContentAndReturnCode& rhs) MAIDSAFE_NOEXCEPT;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MESSAGE_H_
