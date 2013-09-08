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
