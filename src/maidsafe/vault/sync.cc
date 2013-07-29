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

#include "maidsafe/vault/sync.h"

#include "maidsafe/vault/maid_manager/metadata.h"


namespace maidsafe {

namespace vault {

template<>
template<>
void Sync<MaidManager::UnresolvedCreateAccount>::AddUnresolvedAction(
    GroupDb<MaidManager>& database,
    const MaidManager::UnresolvedCreateAccount& unresolved_action) {
  auto resolved_action(AddAction(unresolved_action));
  if (resolved_action) {
    MaidManager::Metadata metadata;
    database.AddGroup(resolved_action->key.group_name, metadata);
  }
}

template<>
template<>
void Sync<MaidManager::UnresolvedRemoveAccount>::AddUnresolvedAction(
    GroupDb<MaidManager>& database,
    const MaidManager::UnresolvedRemoveAccount& unresolved_action) {
  auto resolved_action(AddAction(unresolved_action));
  if (resolved_action)
    database.DeleteGroup(resolved_action->key.group_name);
}

template<>
template<>
void Sync<MaidManager::UnresolvedPut>::AddUnresolvedAction(
    GroupDb<MaidManager>& database,
    const MaidManager::UnresolvedPut& unresolved_action) {
  auto resolved_action(AddAction(unresolved_action));
  if (resolved_action)
    database.Commit(resolved_action->key, resolved_action->action);
}

template<>
template<>
void Sync<MaidManager::UnresolvedDelete>::AddUnresolvedAction(
    GroupDb<MaidManager>& database,
    const MaidManager::UnresolvedDelete& unresolved_action) {
  auto resolved_action(AddAction(unresolved_action));
  if (resolved_action)
    database.Commit(resolved_action->key, resolved_action->action);
}

template<>
template<>
void Sync<MaidManager::UnresolvedRegisterPmid>::AddUnresolvedAction(
    GroupDb<MaidManager>& database,
    const MaidManager::UnresolvedRegisterPmid& unresolved_action) {
  auto resolved_action(AddAction(unresolved_action));
  if (resolved_action)
    database.Commit(resolved_action->key.group_name, resolved_action->action);
}

template<>
template<>
void Sync<MaidManager::UnresolvedUnregisterPmid>::AddUnresolvedAction(
    GroupDb<MaidManager>& database,
    const MaidManager::UnresolvedUnregisterPmid& unresolved_action) {
  auto resolved_action(AddAction(unresolved_action));
  if (resolved_action)
    database.Commit(resolved_action->key.group_name, resolved_action->action);
}

template<>
template<>
void Sync<PmidManager::UnresolvedPut>::AddUnresolvedAction(
    GroupDb<PmidManager>& database,
    const PmidManager::UnresolvedPut& unresolved_action) {
  auto resolved_action(AddAction(unresolved_action));
  if (resolved_action)
    database.Commit(resolved_action->key, resolved_action->action);
}

template<>
template<>
void Sync<PmidManager::UnresolvedDelete>::AddUnresolvedAction(
    GroupDb<PmidManager>& database,
    const PmidManager::UnresolvedDelete& unresolved_action) {
  auto resolved_action(AddAction(unresolved_action));
  if (resolved_action)
    database.Commit(resolved_action->key, resolved_action->action);
}

template<>
template<>
void Sync<PmidManager::UnresolvedGetPmidTotals>::AddUnresolvedAction(
    GroupDb<PmidManager>& database,
    const PmidManager::UnresolvedGetPmidTotals& unresolved_action) {
  auto resolved_action(AddAction(unresolved_action));
  if (resolved_action)
    database.Commit(resolved_action->key.group_name, resolved_action->action);
}

}  // namespace vault

}  // namespace maidsafe
