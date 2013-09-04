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
#include "maidsafe/routing/parameters.h"

#include "maidsafe/vault/types.h"
#include "maidsafe/vault/data_manager/data_manager.h"
#include "maidsafe/vault/version_manager/version_manager.h"


namespace maidsafe {

namespace vault {

template<typename T>
class Accumulator;

template <typename ValidateSender,
          typename AccumulatorType,
          typename Checker,
          typename Handler>
struct OperationHandler {
 public:
  OperationHandler(ValidateSender validate_sender_in,
                   AccumulatorType& accumulator_in,
                   Checker checker_in,
                   Handler handler_in,
                   std::mutex& mutex)
      : validate_sender(validate_sender_in),
        accumulator(accumulator_in),
        checker(checker_in),
        handler(handler_in) {}

  template<typename MessageType, typename Sender, typename Receiver>
  void operator() (const MessageType& message, const Sender& sender, const Receiver& receiver) {
    if (!validator(message, sender))
      return;
    {
      std::lock_guard<std::mutex> lock(mutex);
        if (accumulator.CheckHandled(message))
          return;
      if (accumulator.AddPendingRequest(message, sender, checker) !=
              Accumulator<typename AccumulatorType::type>::AddResult::kSuccess)
        return;
    }
    handler(message, sender, receiver);
  }

 private:
  ValidateSender validate_sender;
  AccumulatorType& accumulator;
  Checker checker;
  Handler handler;
  std::mutex& mutex;
};

template <typename MessageType>
struct HandlerType {
  typedef std::function<void(const MessageType&,
                             const typename MessageType::Sender&,
                             const typename MessageType::Receiver&)> type;
};

template <typename MessageType>
struct ValidateSenderType {
  typedef std::function<bool(const MessageType&,
                             const typename MessageType::Sender&)> type;
};


template <typename MessageType,
          typename AccumulatorVariantType>
struct OperationHandlerWrapper {
  typedef OperationHandler<typename ValidateSenderType<MessageType>::type,
                           Accumulator<AccumulatorVariantType>,
                           typename Accumulator<AccumulatorVariantType>::AddCheckerFunctor,
                           typename HandlerType<MessageType>::type> TypedOperationHandler;

  OperationHandlerWrapper(Accumulator<AccumulatorVariantType>& accumulator,
                          typename ValidateSenderType<MessageType>::type validate_sender,
                          typename Accumulator<AccumulatorVariantType>::AddCheckerFunctor checker,
                          typename HandlerType<MessageType>::type handler,
                          std::mutex& mutex)
      : typed_operation_handler(validate_sender, accumulator, checker, handler, mutex) {}

  void operator() (const MessageType& message,
                   const typename MessageType::Sender& sender,
                   const typename MessageType::Receiver& receiver) {
    typed_operation_handler(message, sender, receiver);
  }

 private:
  TypedOperationHandler typed_operation_handler;
};



namespace detail {

void InitialiseDirectory(const boost::filesystem::path& directory);
//bool ShouldRetry(routing::Routing& routing, const nfs::Message& message);

template<typename Data>
bool IsDataElement(const typename Data::Name& name, const DataNameVariant& data_name_variant);

//void SendReply(const nfs::Message& original_message,
//               const maidsafe_error& return_code,
//               const routing::ReplyFunctor& reply_functor);

template<typename AccountSet, typename Account>
typename Account::serialised_type GetSerialisedAccount(
    std::mutex& mutex,
    const AccountSet& accounts,
    const typename Account::Name& account_name);

template<typename AccountSet, typename Account>
typename Account::serialised_info_type GetSerialisedAccountSyncInfo(
    std::mutex& mutex,
    const AccountSet& accounts,
    const typename Account::Name& account_name);

// To be moved to Routing
bool operator ==(const routing::GroupSource& lhs,  const routing::GroupSource& rhs);

/* Commented by Mahmoud on 2 Sep -- It may be of no use any more
// Returns true if the required successful request count has been reached
template<typename Accumulator>
bool AddResult(const nfs::Message& message,
               const routing::ReplyFunctor& reply_functor,
               const maidsafe_error& return_code,
               Accumulator& accumulator,
               std::mutex& accumulator_mutex,
               int requests_required);
*/

template <typename T>
struct RequiredRequests {
  static uint16_t Value() {
    return  routing::Parameters::node_group_size - 1;
  }
};

}  // namespace detail


//template<typename Message>
//inline bool FromMaidManager(const Message& message);
//
//template<typename Message>
//inline bool FromDataManager(const Message& message);
//
//template<typename Message>
//inline bool FromPmidManager(const Message& message);
//
//template<typename Message>
//inline bool FromDataHolder(const Message& message);
//
//template<typename Message>
//inline bool FromClientMaid(const Message& message);
//
//template<typename Message>
//inline bool FromClientMpid(const Message& message);
//
//template<typename Message>
//inline bool FromOwnerDirectoryManager(const Message& message);
//
//template<typename Message>
//inline bool FromGroupDirectoryManager(const Message& message);
//
//template<typename Message>
//inline bool FromWorldDirectoryManager(const Message& message);
//
//template<typename Message>
//inline bool FromDataGetter(const Message& message);
//
//template<typename Message>
//inline bool FromVersionManager(const nfs::Message& message);

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
