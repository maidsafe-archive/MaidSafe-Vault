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

#ifndef MAIDSAFE_VAULT_SERVICE_H_
#define MAIDSAFE_VAULT_SERVICE_H_

#include "boost/variant/static_visitor.hpp"
#include "boost/variant/variant.hpp"

#include "maidsafe/passport/types.h"
#include "maidsafe/nfs/message_wrapper.h"
#include "maidsafe/nfs/message_wrapper_variant.h"
#include "maidsafe/nfs/public_key_getter.h"
#include "maidsafe/nfs/types.h"


namespace maidsafe {

namespace vault {

template<typename PersonaService, typename T>
class PersonaDemuxer : public boost::static_visitor<> {
 public:
  PersonaDemuxer(PersonaService& persona_service, const typename T::Sender& sender,
                 const typename T::Receiver& receiver)
    : persona_service_(persona_service),
      sender_(sender),
      receiver_(receiver) {}
  template<typename Message>
  void operator()(const Message& message) const {
    persona_service_.HandleMessage(message, sender_, receiver_);
  }
 private:
  PersonaService& persona_service_;
  const typename T::Sender& sender_;
  const typename T::Receiver& receiver_;
};

template<typename PersonaService>
class Service {
 public:
  typedef typename PersonaService::Messages Messages;

  Service(const passport::Pmid& pmid, routing::Routing& routing)
      : impl_(pmid, routing) {}

  template<typename T>
  void HandleMessage(const nfs::TypeErasedMessageWrapper& message,
                     const typename T::Sender& sender,
                     const typename T::Receiver& receiver) {
    auto variant_message(nfs::GetVariant<Messages>(message));
    static const PersonaDemuxer<PersonaService, T> demuxer(impl_, sender, receiver);
    return boost::apply_visitor(demuxer, variant_message);
  }

 private:
  PersonaService impl_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_SERVICE_H_
