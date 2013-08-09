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

#include "maidsafe/vault/demultiplexer.h"

#include "maidsafe/common/log.h"
#include "maidsafe/passport/types.h"
#include "maidsafe/data_types/data_type_values.h"
#include "maidsafe/data_types/data_name_variant.h"
//#include "maidsafe/nfs/reply.h"

#include "maidsafe/vault/pmid_node/service.h"
#include "maidsafe/vault/maid_manager/service.h"
#include "maidsafe/vault/data_manager/service.h"
#include "maidsafe/vault/pmid_manager/service.h"
#include "maidsafe/vault/version_manager/service.h"


namespace maidsafe {

namespace vault {

namespace {

Demultiplexer::Demultiplexer(MaidManagerService& maid_manager_service,
                             VersionManagerService& version_manager_service,
                             DataManagerService& data_manager_service,
                             PmidManagerService& pmid_manager_service,
                             PmidNodeService& pmid_node)
    : maid_manager_service_(maid_manager_service),
      version_manager_service_(version_manager_service),
      data_manager_service_(data_manager_service),
      pmid_manager_service_(pmid_manager_service),
      pmid_node_(pmid_node) {}


//bool Demultiplexer::GetFromCache(std::string& serialised_message) {
//  try {
//    nfs::MessageWrapper request_message_wrapper(
//        (nfs::Message::serialised_type((NonEmptyString(serialised_message)))));
//    nfs::Message request_message(
//        (request_message_wrapper.serialised_inner_message<nfs::Message>()));
//    auto cached_content(HandleGetFromCache(request_message));
//    if (cached_content.IsInitialised()) {
//      nfs::Message response_message(
//          request_message.destination_persona(),
//          request_message.source(),
//          nfs::Message::Data(*request_message.data().type,
//                             request_message.data().name,
//                             cached_content,
//                             request_message.data().action));
//      nfs::MessageWrapper message_wrapper(response_message.Serialise());
//      serialised_message = message_wrapper.Serialise()->string();
//      return true;
//    }
//  }
//  catch(const std::exception& ex) {
//    LOG(kError) << "Caught exception on handling get from cache request: " << ex.what();
//  }
//  return false;
//}

//NonEmptyString Demultiplexer::HandleGetFromCache(const nfs::Message& message) {
//  if (!message.data().type)
//    return NonEmptyString();

//  switch (*message.data().type) {
//    case DataTagValue::kAnmidValue: {
//      typedef is_maidsafe_data<DataTagValue::kAnmidValue>::data_type data_type;
//      return pmid_node_.GetFromCache<data_type>(message);
//    }
//    case DataTagValue::kAnsmidValue: {
//      typedef is_maidsafe_data<DataTagValue::kAnsmidValue>::data_type data_type;
//      return pmid_node_.GetFromCache<data_type>(message);
//    }
//    case DataTagValue::kAntmidValue: {
//      typedef is_maidsafe_data<DataTagValue::kAntmidValue>::data_type data_type;
//      return pmid_node_.GetFromCache<data_type>(message);
//    }
//    case DataTagValue::kAnmaidValue: {
//      typedef is_maidsafe_data<DataTagValue::kAnmaidValue>::data_type data_type;
//      return pmid_node_.GetFromCache<data_type>(message);
//    }
//    case DataTagValue::kMaidValue: {
//      typedef is_maidsafe_data<DataTagValue::kMaidValue>::data_type data_type;
//      return pmid_node_.GetFromCache<data_type>(message);
//    }
//    case DataTagValue::kPmidValue: {
//      typedef is_maidsafe_data<DataTagValue::kPmidValue>::data_type data_type;
//      return pmid_node_.GetFromCache<data_type>(message);
//    }
//    case DataTagValue::kMidValue: {
//      typedef is_maidsafe_data<DataTagValue::kMidValue>::data_type data_type;
//      return pmid_node_.GetFromCache<data_type>(message);
//    }
//    case DataTagValue::kSmidValue: {
//      typedef is_maidsafe_data<DataTagValue::kSmidValue>::data_type data_type;
//      return pmid_node_.GetFromCache<data_type>(message);
//    }
//    case DataTagValue::kTmidValue: {
//      typedef is_maidsafe_data<DataTagValue::kTmidValue>::data_type data_type;
//      return pmid_node_.GetFromCache<data_type>(message);
//    }
//    case DataTagValue::kAnmpidValue: {
//      typedef is_maidsafe_data<DataTagValue::kAnmpidValue>::data_type data_type;
//      return pmid_node_.GetFromCache<data_type>(message);
//    }
//    case DataTagValue::kMpidValue: {
//      typedef is_maidsafe_data<DataTagValue::kMpidValue>::data_type data_type;
//      return pmid_node_.GetFromCache<data_type>(message);
//    }
//    case DataTagValue::kImmutableDataValue: {
//      typedef is_maidsafe_data<DataTagValue::kImmutableDataValue>::data_type data_type;
//      return pmid_node_.GetFromCache<data_type>(message);
//    }
//    case DataTagValue::kOwnerDirectoryValue: {
//      typedef is_maidsafe_data<DataTagValue::kOwnerDirectoryValue>::data_type data_type;
//      return pmid_node_.GetFromCache<data_type>(message);
//    }
//    case DataTagValue::kGroupDirectoryValue: {
//      typedef is_maidsafe_data<DataTagValue::kGroupDirectoryValue>::data_type data_type;
//      return pmid_node_.GetFromCache<data_type>(message);
//    }
//    case DataTagValue::kWorldDirectoryValue: {
//      typedef is_maidsafe_data<DataTagValue::kWorldDirectoryValue>::data_type data_type;
//      return pmid_node_.GetFromCache<data_type>(message);
//    }
//    default:
//      LOG(kError) << "Unhandled data type";
//  }
//  return NonEmptyString();
//}

//void Demultiplexer::StoreInCache(const std::string& serialised_message) {
//  try {
//    nfs::MessageWrapper message_wrapper(
//        (nfs::Message::serialised_type((NonEmptyString(serialised_message)))));
//    nfs::Message message((message_wrapper.serialised_inner_message<nfs::Message>()));
//    HandleStoreInCache(message);
//  }
//  catch(const std::exception& ex) {
//    LOG(kError) << "Caught exception on handling store in cache request: " << ex.what();
//  }
//}

//void Demultiplexer::HandleStoreInCache(const nfs::Message& message) {
//  if (!message.data().type)
//    return;

//  switch (*message.data().type) {
//    case DataTagValue::kAnmidValue: {
//      typedef is_maidsafe_data<DataTagValue::kAnmidValue>::data_type data_type;
//      return pmid_node_.StoreInCache<data_type>(message);
//    }
//    case DataTagValue::kAnsmidValue: {
//      typedef is_maidsafe_data<DataTagValue::kAnsmidValue>::data_type data_type;
//      return pmid_node_.StoreInCache<data_type>(message);
//    }
//    case DataTagValue::kAntmidValue: {
//      typedef is_maidsafe_data<DataTagValue::kAntmidValue>::data_type data_type;
//      return pmid_node_.StoreInCache<data_type>(message);
//    }
//    case DataTagValue::kAnmaidValue: {
//      typedef is_maidsafe_data<DataTagValue::kAnmaidValue>::data_type data_type;
//      return pmid_node_.StoreInCache<data_type>(message);
//    }
//    case DataTagValue::kMaidValue: {
//      typedef is_maidsafe_data<DataTagValue::kMaidValue>::data_type data_type;
//      return pmid_node_.StoreInCache<data_type>(message);
//    }
//    case DataTagValue::kPmidValue: {
//      typedef is_maidsafe_data<DataTagValue::kPmidValue>::data_type data_type;
//      return pmid_node_.StoreInCache<data_type>(message);
//    }
//    case DataTagValue::kMidValue: {
//      typedef is_maidsafe_data<DataTagValue::kMidValue>::data_type data_type;
//      return pmid_node_.StoreInCache<data_type>(message);
//    }
//    case DataTagValue::kSmidValue: {
//      typedef is_maidsafe_data<DataTagValue::kSmidValue>::data_type data_type;
//      return pmid_node_.StoreInCache<data_type>(message);
//    }
//    case DataTagValue::kTmidValue: {
//      typedef is_maidsafe_data<DataTagValue::kTmidValue>::data_type data_type;
//      return pmid_node_.StoreInCache<data_type>(message);
//    }
//    case DataTagValue::kAnmpidValue: {
//      typedef is_maidsafe_data<DataTagValue::kAnmpidValue>::data_type data_type;
//      return pmid_node_.StoreInCache<data_type>(message);
//    }
//    case DataTagValue::kMpidValue: {
//      typedef is_maidsafe_data<DataTagValue::kMpidValue>::data_type data_type;
//      return pmid_node_.StoreInCache<data_type>(message);
//    }
//    case DataTagValue::kImmutableDataValue: {
//      typedef is_maidsafe_data<DataTagValue::kImmutableDataValue>::data_type data_type;
//      return pmid_node_.StoreInCache<data_type>(message);
//    }
//    case DataTagValue::kOwnerDirectoryValue: {
//      typedef is_maidsafe_data<DataTagValue::kOwnerDirectoryValue>::data_type data_type;
//      return pmid_node_.StoreInCache<data_type>(message);
//    }
//    case DataTagValue::kGroupDirectoryValue: {
//      typedef is_maidsafe_data<DataTagValue::kGroupDirectoryValue>::data_type data_type;
//      return pmid_node_.StoreInCache<data_type>(message);
//    }
//    case DataTagValue::kWorldDirectoryValue: {
//      typedef is_maidsafe_data<DataTagValue::kWorldDirectoryValue>::data_type data_type;
//      return pmid_node_.StoreInCache<data_type>(message);
//    }
//    default:
//      LOG(kError) << "Unhandled data type";
//  }
//}

}  // namespace vault

}  // namespace maidsafe
