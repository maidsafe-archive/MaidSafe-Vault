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

#ifndef MAIDSAFE_VAULT_MAID_MANAGER_MAID_MANAGER_H_
#define MAIDSAFE_VAULT_MAID_MANAGER_MAID_MANAGER_H_

#include <cstdint>

#include "boost/variant/variant.hpp"

#include "maidsafe/data_types/immutable_data.h"
#include "maidsafe/passport/types.h"

#include "maidsafe/vault/group_key.h"
#include "maidsafe/vault/unresolved_action.h"
#include "maidsafe/vault/maid_manager/value.h"


namespace maidsafe {

namespace vault {

struct ActionPut;
struct ActionDelete;
struct ActionRegisterPmid;
struct ActionUnregisterPmid;

}  // namespace vault

namespace nfs {

template<>
struct PersonaTypes<Persona::kMaidManager> {
  static const Persona persona = Persona::kMaidManager;

  template<typename Data>
  struct Key {
    typedef maidsafe::vault::GroupKey<passport::PublicMaid::name_type, Data, 1> type;
  };

  typedef maidsafe::vault::MaidManagerValue Value;

  // Put
  template<typename Data>
  struct UnresolvedPut {
    typedef maidsafe::vault::UnresolvedAction<typename Key<Data>::type,
                                              maidsafe::vault::ActionPut> type;
  };
  typedef boost::variant<UnresolvedPut<passport::PublicAnmid>::type,
                         UnresolvedPut<passport::PublicAnsmid>::type,
                         UnresolvedPut<passport::PublicAntmid>::type,
                         UnresolvedPut<passport::PublicAnmaid>::type,
                         UnresolvedPut<passport::PublicMaid>::type,
                         UnresolvedPut<passport::PublicPmid>::type,
                         UnresolvedPut<passport::Mid>::type,
                         UnresolvedPut<passport::Smid>::type,
                         UnresolvedPut<passport::Tmid>::type,
                         UnresolvedPut<passport::PublicAnmpid>::type,
                         UnresolvedPut<passport::PublicMpid>::type,
                         UnresolvedPut<ImmutableData>::type> PutVariant;
  // Delete
  template<typename Data>
  struct UnresolvedDelete {
    typedef maidsafe::vault::UnresolvedAction<typename Key<Data>::type,
                                              maidsafe::vault::ActionDelete> type;
  };
  typedef boost::variant<UnresolvedDelete<passport::PublicAnmid>::type,
                         UnresolvedDelete<passport::PublicAnsmid>::type,
                         UnresolvedDelete<passport::PublicAntmid>::type,
                         UnresolvedDelete<passport::PublicAnmaid>::type,
                         UnresolvedDelete<passport::PublicMaid>::type,
                         UnresolvedDelete<passport::PublicPmid>::type,
                         UnresolvedDelete<passport::Mid>::type,
                         UnresolvedDelete<passport::Smid>::type,
                         UnresolvedDelete<passport::Tmid>::type,
                         UnresolvedDelete<passport::PublicAnmpid>::type,
                         UnresolvedDelete<passport::PublicMpid>::type,
                         UnresolvedDelete<ImmutableData>::type> DeleteVariant;
  // RegisterPmid
  template<typename Data>
  struct UnresolvedRegisterPmid {
    typedef maidsafe::vault::UnresolvedAction<typename Key<Data>::type,
                                              maidsafe::vault::ActionRegisterPmid> type;
  };
  typedef boost::variant<UnresolvedRegisterPmid<passport::PublicAnmid>::type,
                         UnresolvedRegisterPmid<passport::PublicAnsmid>::type,
                         UnresolvedRegisterPmid<passport::PublicAntmid>::type,
                         UnresolvedRegisterPmid<passport::PublicAnmaid>::type,
                         UnresolvedRegisterPmid<passport::PublicMaid>::type,
                         UnresolvedRegisterPmid<passport::PublicPmid>::type,
                         UnresolvedRegisterPmid<passport::Mid>::type,
                         UnresolvedRegisterPmid<passport::Smid>::type,
                         UnresolvedRegisterPmid<passport::Tmid>::type,
                         UnresolvedRegisterPmid<passport::PublicAnmpid>::type,
                         UnresolvedRegisterPmid<passport::PublicMpid>::type,
                         UnresolvedRegisterPmid<ImmutableData>::type> RegisterPmidVariant;
  // UnregisterPmid
  template<typename Data>
  struct UnresolvedUnregisterPmid {
    typedef maidsafe::vault::UnresolvedAction<typename Key<Data>::type,
                                              maidsafe::vault::ActionUnregisterPmid> type;
  };
  typedef boost::variant<UnresolvedUnregisterPmid<passport::PublicAnmid>::type,
                         UnresolvedUnregisterPmid<passport::PublicAnsmid>::type,
                         UnresolvedUnregisterPmid<passport::PublicAntmid>::type,
                         UnresolvedUnregisterPmid<passport::PublicAnmaid>::type,
                         UnresolvedUnregisterPmid<passport::PublicMaid>::type,
                         UnresolvedUnregisterPmid<passport::PublicPmid>::type,
                         UnresolvedUnregisterPmid<passport::Mid>::type,
                         UnresolvedUnregisterPmid<passport::Smid>::type,
                         UnresolvedUnregisterPmid<passport::Tmid>::type,
                         UnresolvedUnregisterPmid<passport::PublicAnmpid>::type,
                         UnresolvedUnregisterPmid<passport::PublicMpid>::type,
                         UnresolvedUnregisterPmid<ImmutableData>::type> UnregisterPmidVariant;

  enum class Action : int32_t {
    kPut,
    kDelete,
    kRegisterPmid,
    kUnregisterPmid,
    kSync,
    kAccountTransfer
  };
};

}  // namespace nfs


namespace vault {

typedef nfs::PersonaTypes<nfs::Persona::kMaidManager> MaidManager;

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MAID_MANAGER_MAID_MANAGER_H_
