/* Copyright 2012 MaidSafe.net limited

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

#ifndef MAIDSAFE_VAULT_UTILS_H_
#define MAIDSAFE_VAULT_UTILS_H_

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "boost/filesystem/path.hpp"
#include "leveldb/db.h"

#include "maidsafe/common/error.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/routing/routing_api.h"
#include "maidsafe/nfs/message.h"

#include "maidsafe/vault/types.h"
#include "maidsafe/vault/data_manager/data_manager.h"
#include "maidsafe/vault/version_manager/version_manager.h"


namespace maidsafe {

namespace vault {

namespace detail {

void InitialiseDirectory(const boost::filesystem::path& directory);
bool ShouldRetry(routing::Routing& routing, const nfs::Message& message);

template<typename Data>
bool IsDataElement(const typename Data::name_type& name,
                   const DataNameVariant& data_name_variant);

void SendReply(const nfs::Message& original_message,
               const maidsafe_error& return_code,
               const routing::ReplyFunctor& reply_functor);

template<typename AccountSet, typename Account>
typename Account::serialised_type GetSerialisedAccount(
    std::mutex& mutex,
    const AccountSet& accounts,
    const typename Account::name_type& account_name);

template<typename AccountSet, typename Account>
typename Account::serialised_info_type GetSerialisedAccountSyncInfo(
    std::mutex& mutex,
    const AccountSet& accounts,
    const typename Account::name_type& account_name);
// Returns true if the required successful request count has been reached
template<typename Accumulator>
bool AddResult(const nfs::Message& message,
               const routing::ReplyFunctor& reply_functor,
               const maidsafe_error& return_code,
               Accumulator& accumulator,
               std::mutex& accumulator_mutex,
               int requests_required);

template<int width>
std::string ToFixedWidthString(uint32_t number);

template<int width>
uint32_t FromFixedWidthString(const std::string& number_as_string);

}  // namespace detail

struct CheckHoldersResult {
  std::vector<NodeId> new_holders;
  std::vector<NodeId> old_holders;
  routing::GroupRangeStatus proximity_status;
};

CheckHoldersResult CheckHolders(const routing::MatrixChange& matrix_change,
                                const NodeId& this_id,
                                const NodeId& target);

template<typename Message>
inline bool FromMaidManager(const Message& message);

template<typename Message>
inline bool FromDataManager(const Message& message);

template<typename Message>
inline bool FromPmidManager(const Message& message);

template<typename Message>
inline bool FromDataHolder(const Message& message);

template<typename Message>
inline bool FromClientMaid(const Message& message);

template<typename Message>
inline bool FromClientMpid(const Message& message);

template<typename Message>
inline bool FromOwnerDirectoryManager(const Message& message);

template<typename Message>
inline bool FromGroupDirectoryManager(const Message& message);

template<typename Message>
inline bool FromWorldDirectoryManager(const Message& message);

template<typename Message>
inline bool FromDataGetter(const Message& message);

template<typename Message>
inline bool FromVersionManager(const nfs::Message& message);


template<typename Persona>
typename Persona::DbKey GetKeyFromMessage(const nfs::Message& message) {
  if (!message.data().type)
    ThrowError(CommonErrors::parsing_error);
  return GetDataNameVariant(*message.data().type, message.data().name);
}

template<>
typename VersionManager::DbKey
         GetKeyFromMessage<VersionManager>(const nfs::Message& message);

template<typename PersonaTypes>
typename PersonaTypes::RecordName GetRecordName(const typename PersonaTypes::DbKey& db_key);

template<>
typename DataManager::RecordName GetRecordName<DataManager>(
    const typename DataManager::DbKey& db_key);

template<>
typename VersionManager::RecordName GetRecordName<VersionManager>(
    const typename VersionManager::DbKey& db_key);

std::unique_ptr<leveldb::DB> InitialiseLevelDb(const boost::filesystem::path& db_path);

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/utils-inl.h"

#endif  // MAIDSAFE_VAULT_UTILS_H_
