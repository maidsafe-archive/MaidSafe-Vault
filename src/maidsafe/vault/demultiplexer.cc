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
#include "maidsafe/nfs/reply.h"

#include "maidsafe/vault/pmid_node/pmid_node_service.h"
#include "maidsafe/vault/maid_manager/maid_manager_service.h"
#include "maidsafe/vault/metadata_manager/metadata_manager_service.h"
#include "maidsafe/vault/pmid_manager/pmid_manager_service.h"
#include "maidsafe/vault/version_manager/version_manager_service.h"


namespace maidsafe {

namespace vault {

namespace {

template<typename Persona>
void HandleDataType(const nfs::Message& message,
                    const routing::ReplyFunctor& reply_functor,
                    Persona& persona) {
  if (!message.data().type)
    return persona.HandleMessage(message, reply_functor);

  switch (*message.data().type) {
    case DataTagValue::kAnmidValue: {
      typedef is_maidsafe_data<DataTagValue::kAnmidValue>::data_type data_type;
      return persona.template HandleMessage<data_type>(message, reply_functor);
    }
    case DataTagValue::kAnsmidValue: {
      typedef is_maidsafe_data<DataTagValue::kAnsmidValue>::data_type data_type;
      return persona.template HandleMessage<data_type>(message, reply_functor);
    }
    case DataTagValue::kAntmidValue: {
      typedef is_maidsafe_data<DataTagValue::kAntmidValue>::data_type data_type;
      return persona.template HandleMessage<data_type>(message, reply_functor);
    }
    case DataTagValue::kAnmaidValue: {
      typedef is_maidsafe_data<DataTagValue::kAnmaidValue>::data_type data_type;
      return persona.template HandleMessage<data_type>(message, reply_functor);
    }
    case DataTagValue::kMaidValue: {
      typedef is_maidsafe_data<DataTagValue::kMaidValue>::data_type data_type;
      return persona.template HandleMessage<data_type>(message, reply_functor);
    }
    case DataTagValue::kPmidValue: {
      typedef is_maidsafe_data<DataTagValue::kPmidValue>::data_type data_type;
      return persona.template HandleMessage<data_type>(message, reply_functor);
    }
    case DataTagValue::kMidValue: {
      typedef is_maidsafe_data<DataTagValue::kMidValue>::data_type data_type;
      return persona.template HandleMessage<data_type>(message, reply_functor);
    }
    case DataTagValue::kSmidValue: {
      typedef is_maidsafe_data<DataTagValue::kSmidValue>::data_type data_type;
      return persona.template HandleMessage<data_type>(message, reply_functor);
    }
    case DataTagValue::kTmidValue: {
      typedef is_maidsafe_data<DataTagValue::kTmidValue>::data_type data_type;
      return persona.template HandleMessage<data_type>(message, reply_functor);
    }
    case DataTagValue::kAnmpidValue: {
      typedef is_maidsafe_data<DataTagValue::kAnmpidValue>::data_type data_type;
      return persona.template HandleMessage<data_type>(message, reply_functor);
    }
    case DataTagValue::kMpidValue: {
      typedef is_maidsafe_data<DataTagValue::kMpidValue>::data_type data_type;
      return persona.template HandleMessage<data_type>(message, reply_functor);
    }
    case DataTagValue::kImmutableDataValue: {
      typedef is_maidsafe_data<DataTagValue::kImmutableDataValue>::data_type data_type;
      return persona.template HandleMessage<data_type>(message, reply_functor);
    }
    case DataTagValue::kOwnerDirectoryValue: {
      typedef is_maidsafe_data<DataTagValue::kOwnerDirectoryValue>::data_type data_type;
      return persona.template HandleMessage<data_type>(message, reply_functor);
    }
    case DataTagValue::kGroupDirectoryValue: {
      typedef is_maidsafe_data<DataTagValue::kGroupDirectoryValue>::data_type data_type;
      return persona.template HandleMessage<data_type>(message, reply_functor);
    }
    case DataTagValue::kWorldDirectoryValue: {
      typedef is_maidsafe_data<DataTagValue::kWorldDirectoryValue>::data_type data_type;
      return persona.template HandleMessage<data_type>(message, reply_functor);
    }
    default:
      LOG(kError) << "Unhandled data type";
  }
}

}  // unnamed namespace


Demultiplexer::Demultiplexer(MaidAccountHolderService& maid_manager_service,
                             VersionManagerService& version_manager_service,
                             MetadataManagerService& metadata_manager_service,
                             PmidAccountHolderService& pmid_manager_service,
                             DataHolderService& pmid_node)
    : maid_manager_service_(maid_manager_service),
      version_manager_service_(version_manager_service),
      metadata_manager_service_(metadata_manager_service),
      pmid_manager_service_(pmid_manager_service),
      pmid_node_(pmid_node) {}

void Demultiplexer::HandleMessage(const std::string& serialised_message,
                                  const routing::ReplyFunctor& reply_functor) {
  try {
    nfs::MessageWrapper message_wrapper(
        (nfs::MessageWrapper::serialised_type((NonEmptyString(serialised_message)))));
    nfs::Message message(message_wrapper.serialised_inner_message<nfs::Message>());
    return PersonaHandleMessage(message, reply_functor);
  }
  catch(const maidsafe_error& error) {
    LOG(kError) << "Caught exception on handling new message: " << error.what();
    if (reply_functor && !serialised_message.empty()) {
      nfs::Reply reply(error, NonEmptyString(serialised_message));
      reply_functor(reply.Serialise()->string());
    }
  }
  catch(const std::exception& ex) {
    LOG(kError) << "Caught exception on handling new message: " << ex.what();
    if (reply_functor && !serialised_message.empty()) {
      // TODO(Fraser#5#): 2013-04-26 - Consider sending actual std exception back.
      nfs::Reply reply(CommonErrors::unknown, NonEmptyString(serialised_message));
      reply_functor(reply.Serialise()->string());
    }
  }
}

template<>
void Demultiplexer::PersonaHandleMessage<nfs::Message>(
    const nfs::Message& message,
    const routing::ReplyFunctor& reply_functor) {
  switch (message.destination_persona()) {
    case nfs::Persona::kMaidAccountHolder:
      return HandleDataType<MaidAccountHolderService>(message, reply_functor,
                                                      maid_manager_service_);
    case nfs::Persona::kMetadataManager:
      return HandleDataType<MetadataManagerService>(message, reply_functor,
                                                    metadata_manager_service_);
    case nfs::Persona::kPmidAccountHolder:
      return HandleDataType<PmidAccountHolderService>(message, reply_functor,
                                                      pmid_manager_service_);
    case nfs::Persona::kDataHolder:
      return HandleDataType<DataHolderService>(message, reply_functor, pmid_node_);
    default:
      LOG(kError) << "Unhandled Persona";
  }
}

bool Demultiplexer::GetFromCache(std::string& serialised_message) {
  try {
    nfs::MessageWrapper request_message_wrapper(
        (nfs::Message::serialised_type((NonEmptyString(serialised_message)))));
    nfs::Message request_message(
        (request_message_wrapper.serialised_inner_message<nfs::Message>()));
    auto cached_content(HandleGetFromCache(request_message));
    if (cached_content.IsInitialised()) {
      nfs::Message response_message(
          request_message.destination_persona(),
          request_message.source(),
          nfs::Message::Data(*request_message.data().type,
                             request_message.data().name,
                             cached_content,
                             request_message.data().action));
      nfs::MessageWrapper message_wrapper(response_message.Serialise());
      serialised_message = message_wrapper.Serialise()->string();
      return true;
    }
  }
  catch(const std::exception& ex) {
    LOG(kError) << "Caught exception on handling get from cache request: " << ex.what();
  }
  return false;
}

NonEmptyString Demultiplexer::HandleGetFromCache(const nfs::Message& message) {
  if (!message.data().type)
    return NonEmptyString();

  switch (*message.data().type) {
    case DataTagValue::kAnmidValue: {
      typedef is_maidsafe_data<DataTagValue::kAnmidValue>::data_type data_type;
      return pmid_node_.GetFromCache<data_type>(message);
    }
    case DataTagValue::kAnsmidValue: {
      typedef is_maidsafe_data<DataTagValue::kAnsmidValue>::data_type data_type;
      return pmid_node_.GetFromCache<data_type>(message);
    }
    case DataTagValue::kAntmidValue: {
      typedef is_maidsafe_data<DataTagValue::kAntmidValue>::data_type data_type;
      return pmid_node_.GetFromCache<data_type>(message);
    }
    case DataTagValue::kAnmaidValue: {
      typedef is_maidsafe_data<DataTagValue::kAnmaidValue>::data_type data_type;
      return pmid_node_.GetFromCache<data_type>(message);
    }
    case DataTagValue::kMaidValue: {
      typedef is_maidsafe_data<DataTagValue::kMaidValue>::data_type data_type;
      return pmid_node_.GetFromCache<data_type>(message);
    }
    case DataTagValue::kPmidValue: {
      typedef is_maidsafe_data<DataTagValue::kPmidValue>::data_type data_type;
      return pmid_node_.GetFromCache<data_type>(message);
    }
    case DataTagValue::kMidValue: {
      typedef is_maidsafe_data<DataTagValue::kMidValue>::data_type data_type;
      return pmid_node_.GetFromCache<data_type>(message);
    }
    case DataTagValue::kSmidValue: {
      typedef is_maidsafe_data<DataTagValue::kSmidValue>::data_type data_type;
      return pmid_node_.GetFromCache<data_type>(message);
    }
    case DataTagValue::kTmidValue: {
      typedef is_maidsafe_data<DataTagValue::kTmidValue>::data_type data_type;
      return pmid_node_.GetFromCache<data_type>(message);
    }
    case DataTagValue::kAnmpidValue: {
      typedef is_maidsafe_data<DataTagValue::kAnmpidValue>::data_type data_type;
      return pmid_node_.GetFromCache<data_type>(message);
    }
    case DataTagValue::kMpidValue: {
      typedef is_maidsafe_data<DataTagValue::kMpidValue>::data_type data_type;
      return pmid_node_.GetFromCache<data_type>(message);
    }
    case DataTagValue::kImmutableDataValue: {
      typedef is_maidsafe_data<DataTagValue::kImmutableDataValue>::data_type data_type;
      return pmid_node_.GetFromCache<data_type>(message);
    }
    case DataTagValue::kOwnerDirectoryValue: {
      typedef is_maidsafe_data<DataTagValue::kOwnerDirectoryValue>::data_type data_type;
      return pmid_node_.GetFromCache<data_type>(message);
    }
    case DataTagValue::kGroupDirectoryValue: {
      typedef is_maidsafe_data<DataTagValue::kGroupDirectoryValue>::data_type data_type;
      return pmid_node_.GetFromCache<data_type>(message);
    }
    case DataTagValue::kWorldDirectoryValue: {
      typedef is_maidsafe_data<DataTagValue::kWorldDirectoryValue>::data_type data_type;
      return pmid_node_.GetFromCache<data_type>(message);
    }
    default:
      LOG(kError) << "Unhandled data type";
  }
  return NonEmptyString();
}

void Demultiplexer::StoreInCache(const std::string& serialised_message) {
  try {
    nfs::MessageWrapper message_wrapper(
        (nfs::Message::serialised_type((NonEmptyString(serialised_message)))));
    nfs::Message message((message_wrapper.serialised_inner_message<nfs::Message>()));
    HandleStoreInCache(message);
  }
  catch(const std::exception& ex) {
    LOG(kError) << "Caught exception on handling store in cache request: " << ex.what();
  }
}

void Demultiplexer::HandleStoreInCache(const nfs::Message& message) {
  if (!message.data().type)
    return;

  switch (*message.data().type) {
    case DataTagValue::kAnmidValue: {
      typedef is_maidsafe_data<DataTagValue::kAnmidValue>::data_type data_type;
      return pmid_node_.StoreInCache<data_type>(message);
    }
    case DataTagValue::kAnsmidValue: {
      typedef is_maidsafe_data<DataTagValue::kAnsmidValue>::data_type data_type;
      return pmid_node_.StoreInCache<data_type>(message);
    }
    case DataTagValue::kAntmidValue: {
      typedef is_maidsafe_data<DataTagValue::kAntmidValue>::data_type data_type;
      return pmid_node_.StoreInCache<data_type>(message);
    }
    case DataTagValue::kAnmaidValue: {
      typedef is_maidsafe_data<DataTagValue::kAnmaidValue>::data_type data_type;
      return pmid_node_.StoreInCache<data_type>(message);
    }
    case DataTagValue::kMaidValue: {
      typedef is_maidsafe_data<DataTagValue::kMaidValue>::data_type data_type;
      return pmid_node_.StoreInCache<data_type>(message);
    }
    case DataTagValue::kPmidValue: {
      typedef is_maidsafe_data<DataTagValue::kPmidValue>::data_type data_type;
      return pmid_node_.StoreInCache<data_type>(message);
    }
    case DataTagValue::kMidValue: {
      typedef is_maidsafe_data<DataTagValue::kMidValue>::data_type data_type;
      return pmid_node_.StoreInCache<data_type>(message);
    }
    case DataTagValue::kSmidValue: {
      typedef is_maidsafe_data<DataTagValue::kSmidValue>::data_type data_type;
      return pmid_node_.StoreInCache<data_type>(message);
    }
    case DataTagValue::kTmidValue: {
      typedef is_maidsafe_data<DataTagValue::kTmidValue>::data_type data_type;
      return pmid_node_.StoreInCache<data_type>(message);
    }
    case DataTagValue::kAnmpidValue: {
      typedef is_maidsafe_data<DataTagValue::kAnmpidValue>::data_type data_type;
      return pmid_node_.StoreInCache<data_type>(message);
    }
    case DataTagValue::kMpidValue: {
      typedef is_maidsafe_data<DataTagValue::kMpidValue>::data_type data_type;
      return pmid_node_.StoreInCache<data_type>(message);
    }
    case DataTagValue::kImmutableDataValue: {
      typedef is_maidsafe_data<DataTagValue::kImmutableDataValue>::data_type data_type;
      return pmid_node_.StoreInCache<data_type>(message);
    }
    case DataTagValue::kOwnerDirectoryValue: {
      typedef is_maidsafe_data<DataTagValue::kOwnerDirectoryValue>::data_type data_type;
      return pmid_node_.StoreInCache<data_type>(message);
    }
    case DataTagValue::kGroupDirectoryValue: {
      typedef is_maidsafe_data<DataTagValue::kGroupDirectoryValue>::data_type data_type;
      return pmid_node_.StoreInCache<data_type>(message);
    }
    case DataTagValue::kWorldDirectoryValue: {
      typedef is_maidsafe_data<DataTagValue::kWorldDirectoryValue>::data_type data_type;
      return pmid_node_.StoreInCache<data_type>(message);
    }
    default:
      LOG(kError) << "Unhandled data type";
  }
}

}  // namespace vault

}  // namespace maidsafe
