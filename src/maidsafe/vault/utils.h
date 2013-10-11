/*  Copyright 2012 MaidSafe.net limited

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

#ifndef MAIDSAFE_VAULT_UTILS_H_
#define MAIDSAFE_VAULT_UTILS_H_

#include "leveldb/db.h"

#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/nfs/message_types.h"
#include "maidsafe/vault/key_utils.h"


namespace maidsafe {

namespace vault {

namespace detail {

template <typename T>
DataNameVariant GetNameVariant(const T&);

template <>
DataNameVariant GetNameVariant(const nfs_vault::DataName& data);

template <>
DataNameVariant GetNameVariant(const nfs_vault::DataNameAndContent& data);

template <>
DataNameVariant GetNameVariant(const nfs_vault::DataAndPmidHint& data);

template <>
DataNameVariant GetNameVariant(const nfs_client::DataAndReturnCode& data);

template <>
DataNameVariant GetNameVariant(const nfs_client::DataNameAndContentOrReturnCode& data);

template <>
DataNameVariant GetNameVariant(const nfs_vault::DataNameAndContentOrCheckResult& data);

template <typename MessageType>
struct ValidateSenderType {
  typedef std::function<bool(const MessageType&, const typename MessageType::Sender&)> type;
};

// =============================================================================================

void InitialiseDirectory(const boost::filesystem::path& directory);
// bool ShouldRetry(routing::Routing& routing, const nfs::Message& message);

template <typename Data>
bool IsDataElement(const typename Data::Name& name, const DataNameVariant& data_name_variant);

// void SendReply(const nfs::Message& original_message,
//               const maidsafe_error& return_code,
//               const routing::ReplyFunctor& reply_functor);

template <typename AccountSet, typename Account>
typename Account::serialised_type GetSerialisedAccount(std::mutex& mutex,
                                                       const AccountSet& accounts,
                                                       const typename Account::Name& account_name);

template <typename AccountSet, typename Account>
typename Account::serialised_info_type GetSerialisedAccountSyncInfo(
    std::mutex& mutex, const AccountSet& accounts, const typename Account::Name& account_name);

}  // namespace detail

// template<typename Message>
// inline bool FromMaidManager(const Message& message);
//
// template<typename Message>
// inline bool FromDataManager(const Message& message);
//
// template<typename Message>
// inline bool FromPmidManager(const Message& message);
//
// template<typename Message>
// inline bool FromDataHolder(const Message& message);
//
// template<typename Message>
// inline bool FromClientMaid(const Message& message);
//
// template<typename Message>
// inline bool FromClientMpid(const Message& message);
//
// template<typename Message>
// inline bool FromOwnerDirectoryManager(const Message& message);
//
// template<typename Message>
// inline bool FromGroupDirectoryManager(const Message& message);
//
// template<typename Message>
// inline bool FromWorldDirectoryManager(const Message& message);
//
// template<typename Message>
// inline bool FromDataGetter(const Message& message);
//
// template<typename Message>
// inline bool FromVersionHandler(const nfs::Message& message);

/* Commented by Mahmoud on 2 Sep -- It may be of no use any more
template<typename Persona>
typename Persona::DbKey GetKeyFromMessage(const nfs::Message& message) {
  if (!message.data().type)
    ThrowError(CommonErrors::parsing_error);
  return GetDataNameVariant(*message.data().type, message.data().name);
}
*/
std::unique_ptr<leveldb::DB> InitialiseLevelDb(const boost::filesystem::path& db_path);

}  // namespace vault

}  // namespace maidsafe

#include "maidsafe/vault/utils-inl.h"

#endif  // MAIDSAFE_VAULT_UTILS_H_
