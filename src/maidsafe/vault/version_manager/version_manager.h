

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

#ifndef MAIDSAFE_VAULT_VERSION_MANAGER_VERSION_MANAGER_H_
#define MAIDSAFE_VAULT_VERSION_MANAGER_VERSION_MANAGER_H_

#include <cstdint>

#include "boost/variant/variant.hpp"

#include "maidsafe/data_types/structured_data_versions.h"
#include "maidsafe/data_types/owner_directory.h"
#include "maidsafe/data_types/group_directory.h"
#include "maidsafe/data_types/world_directory.h"

#include "maidsafe/vault/unresolved_action.h"
#include "maidsafe/vault/version_manager/key.h"


namespace maidsafe {

namespace vault {

struct ActionPutVersion;
struct ActionGetVersion;
struct ActionGetBranch;
struct ActionDeleteBranchUntilFork;

}  // namespace vault


namespace nfs {

template<>
struct PersonaTypes<Persona::kVersionManager> {
  static const Persona persona = Persona::kVersionManager;

  template<typename Data>
  struct Key {
    typedef maidsafe::vault::VersionManagerKey<Data> type;
  };

  typedef StructuredDataVersions Value;

  // PutVersion
  template<typename Data>
  struct UnresolvedPutVersion {
    typedef maidsafe::vault::UnresolvedAction<typename Key<Data>::type,
                                              maidsafe::vault::ActionPutVersion> type;
  };
  typedef boost::variant<UnresolvedPutVersion<OwnerDirectory>::type,
                         UnresolvedPutVersion<GroupDirectory>::type,
                         UnresolvedPutVersion<WorldDirectory>::type> PutVersionVariant;
  // GetVersion
  template<typename Data>
  struct UnresolvedGetVersion {
    typedef maidsafe::vault::UnresolvedAction<typename Key<Data>::type,
                                              maidsafe::vault::ActionGetVersion> type;
  };
  typedef boost::variant<UnresolvedGetVersion<OwnerDirectory>::type,
                         UnresolvedGetVersion<GroupDirectory>::type,
                         UnresolvedGetVersion<WorldDirectory>::type> GetVersionVariant;

  // GetBranch
  template<typename Data>
  struct UnresolvedGetBranch {
    typedef maidsafe::vault::UnresolvedAction<typename Key<Data>::type,
                                              maidsafe::vault::ActionGetBranch> type;
  };
  typedef boost::variant<UnresolvedGetBranch<OwnerDirectory>::type,
                         UnresolvedGetBranch<GroupDirectory>::type,
                         UnresolvedGetBranch<WorldDirectory>::type> GetBranchVariant;

  // DeleteBranchUntilFork
  template<typename Data>
  struct UnresolvedDeleteBranchUntilFork {
    typedef maidsafe::vault::UnresolvedAction<typename Key<Data>::type,
                                              maidsafe::vault::ActionDeleteBranchUntilFork> type;
  };
  typedef boost::variant<UnresolvedDeleteBranchUntilFork<OwnerDirectory>::type,
                         UnresolvedDeleteBranchUntilFork<GroupDirectory>::type,
                         UnresolvedDeleteBranchUntilFork<WorldDirectory>::type>
                             DeleteBranchUntilForkVariant;

  enum class Action : int32_t {
    kPut,
    kGet,
    kGetBranch,
    kDeleteBranchUntilFork,
    kSync,
    kAccountTransfer
  };
};

}  // namespace nfs


namespace vault {

typedef nfs::PersonaTypes<nfs::Persona::kVersionManager> VersionManager;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VERSION_MANAGER_VERSION_MANAGER_H_
