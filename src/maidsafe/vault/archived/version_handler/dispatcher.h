/*  Copyright 2013 MaidSafe.net limited

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

#ifndef MAIDSAFE_VAULT_VERSION_HANDLER_DISPATCHER_H_
#define MAIDSAFE_VAULT_VERSION_HANDLER_DISPATCHER_H_

#include <string>
#include <vector>

#include "maidsafe/common/data_types/structured_data_versions.h"

#include "maidsafe/routing/routing_api.h"

#include "maidsafe/vault/version_handler/version_handler.h"
#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

class VersionHandlerDispatcher {
 public:
  explicit VersionHandlerDispatcher(routing::Routing& routing);

  template <typename RequestorType>
  void SendGetVersionsResponse(
      const VersionHandler::Key& key, const std::vector<VersionHandler::VersionName>& versions,
      const RequestorType& requestor, const maidsafe_error& result, nfs::MessageId message_id);

  template <typename RequestorType>
  void SendGetBranchResponse(
      const VersionHandler::Key& key, const std::vector<VersionHandler::VersionName>& versions,
      const RequestorType& requestor, const maidsafe_error& result, nfs::MessageId message_id);

  void SendCreateVersionTreeResponse(const Identity& requestor, const VersionHandler::Key& key,
                                     const maidsafe_error& return_code,
                                     nfs::MessageId message_id);

  void SendPutVersionResponse(const Identity& originator, const VersionHandler::Key& key,
                              const VersionHandler::VersionName& tip_of_tree,
                              const maidsafe_error& return_code, nfs::MessageId message_id);

  void SendSync(const VersionHandler::Key& key, const std::string& serialised_sync);

  void SendAccountTransfer(const NodeId& destination_peer, const std::string& serialised_account);
  void SendAccountQuery(const VersionHandler::Key& key);
  void SendAccountQueryResponse(const std::string& serialised_account,
                                const routing::GroupId& group_id, const NodeId& sender);

  template <typename Message>
  void CheckSourcePersonaType() const;

 private:
  routing::Routing& routing_;
};

template <typename RequestorType>
void VersionHandlerDispatcher::SendGetVersionsResponse(
    const VersionHandler::Key&, const std::vector<VersionHandler::VersionName>&,
    const RequestorType&, const maidsafe_error&, nfs::MessageId) {
  RequestorType::No_generic_handler_is_available__Specialisation_is_required;
}

template <typename RequestorType>
void SendGetBranchResponse(const std::vector<VersionHandler::VersionName>& /*versions*/,
                           const RequestorType& /*requestor*/, const maidsafe_error& /*result*/) {
  RequestorType::No_generic_handler_is_available__Specialisation_is_required;
}

template <>
void VersionHandlerDispatcher::SendGetVersionsResponse(
    const VersionHandler::Key& key, const std::vector<VersionHandler::VersionName>& versions,
    const detail::Requestor<nfs::SourcePersona<maidsafe::nfs::Persona::kDataGetter>>& requestor,
    const maidsafe_error& result, nfs::MessageId message_id);

template <>
void VersionHandlerDispatcher::SendGetVersionsResponse(
    const VersionHandler::Key& key,  const std::vector<VersionHandler::VersionName>& versions,
    const detail::Requestor<nfs::SourcePersona<maidsafe::nfs::Persona::kMaidNode>>& requestor,
    const maidsafe_error& result, nfs::MessageId message_id);

template <>
void VersionHandlerDispatcher::SendGetBranchResponse(
    const VersionHandler::Key& key, const std::vector<VersionHandler::VersionName>& versions,
    const detail::Requestor<nfs::SourcePersona<maidsafe::nfs::Persona::kDataGetter>>& requestor,
    const maidsafe_error& result, nfs::MessageId message_id);

template <>
void VersionHandlerDispatcher::SendGetBranchResponse(
    const VersionHandler::Key& key, const std::vector<VersionHandler::VersionName>& versions,
    const detail::Requestor<nfs::SourcePersona<maidsafe::nfs::Persona::kMaidNode>>& requestor,
    const maidsafe_error& result, nfs::MessageId message_id);

template<typename Message>
void VersionHandlerDispatcher::CheckSourcePersonaType() const {
  static_assert(Message::SourcePersona::value == nfs::Persona::kVersionHandler,
                "The source Persona must be kDataManager.");
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_VERSION_HANDLER_DISPATCHER_H_
