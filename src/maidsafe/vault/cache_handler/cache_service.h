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

#ifndef MAIDSAFE_VAULT_CACHE_SERVICE_H_
#define MAIDSAFE_VAULT_CACHE_SERVICE_H_

#include "maidsafe/vault/cache_handler/service.h"
#include "maidsafe/nfs/service.h"

namespace maidsafe {

namespace nfs {

template <>
template <typename Sender, typename Receiver>
vault::CacheHandlerService::HandleMessageReturnType
Service<vault::CacheHandlerService>::HandleMessage(
    const nfs::TypeErasedMessageWrapper& message, const Sender& sender, const Receiver& receiver) {
    const detail::PersonaDemuxer<vault::CacheHandlerService, Sender, Receiver>
        demuxer(*impl_, sender,receiver);

    static std::is_void<PublicMessages> public_messages_void_state;
    static std::is_void<VaultMessages> vault_messages_void_state;
    return HandleMessage(message, demuxer, public_messages_void_state, vault_messages_void_state);
}

template <>
template <typename Demuxer>
vault::CacheHandlerService::HandleMessageReturnType
Service<vault::CacheHandlerService>::HandlePublicMessage(
    const nfs::TypeErasedMessageWrapper& message, const Demuxer& demuxer) {
  PublicMessages public_variant_message;
  if (!nfs::GetVariantOfCacheable(message, public_variant_message))
    ThrowError(CommonErrors::invalid_parameter);
  return boost::apply_visitor(demuxer, public_variant_message);
}

template <>
template <typename Demuxer>
vault::CacheHandlerService::HandleMessageReturnType
Service<vault::CacheHandlerService>::HandleVaultMessage(
    const nfs::TypeErasedMessageWrapper& message, const Demuxer& demuxer) {
  VaultMessages vault_variant_message;
  if (!vault::GetVariantOfCacheable(message, vault_variant_message))
    ThrowError(CommonErrors::invalid_parameter);
  return boost::apply_visitor(demuxer, vault_variant_message);
}

}  // namespace nfs

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_CACHE_HANDLER_SERVICE_H_
