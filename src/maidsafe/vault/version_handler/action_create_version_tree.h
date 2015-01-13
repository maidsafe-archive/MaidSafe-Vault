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

#ifndef MAIDSAFE_VAULT_VERSION_HANDLER_ACTION_CREATE_VERSION_TREE_H_
#define MAIDSAFE_VAULT_VERSION_HANDLER_ACTION_CREATE_VERSION_TREE_H_

#include <string>

#include "boost/optional.hpp"

#include "maidsafe/vault/version_handler/version_handler.h"
#include "maidsafe/vault/config.h"

namespace maidsafe {

namespace vault {

class VersionHandlerValue;

struct ActionVersionHandlerCreateVersionTree {
  ActionVersionHandlerCreateVersionTree(const StructuredDataVersions::VersionName& version_in,
                                        const Identity& originator_in,
                                        uint32_t max_versions_in, uint32_t max_branches_in,
                                        nfs::MessageId message_id_in);

  explicit ActionVersionHandlerCreateVersionTree(const std::string& serialised_action);
  ActionVersionHandlerCreateVersionTree(const ActionVersionHandlerCreateVersionTree& other);
  ActionVersionHandlerCreateVersionTree(ActionVersionHandlerCreateVersionTree&& other);

  detail::DbAction operator()(std::unique_ptr<VersionHandlerValue>& value);

  std::string Serialise() const;

  static const nfs::MessageAction kActionId = nfs::MessageAction::kCreateVersionTreeRequest;
  StructuredDataVersions::VersionName version;
  uint32_t max_versions, max_branches;
  nfs::MessageId message_id;
  Identity originator;

 private:
  ActionVersionHandlerCreateVersionTree();
  ActionVersionHandlerCreateVersionTree& operator=(ActionVersionHandlerCreateVersionTree other);
};

bool operator==(const ActionVersionHandlerCreateVersionTree& lhs,
                const ActionVersionHandlerCreateVersionTree& rhs);
bool operator!=(const ActionVersionHandlerCreateVersionTree& lhs,
                const ActionVersionHandlerCreateVersionTree& rhs);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VERSION_HANDLER_ACTION_CREATE_VERSION_TREE_H_
