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

#include "maidsafe/data_types/structured_data_versions.h"

#include "maidsafe/routing/routing_api.h"

#include "maidsafe/vault/version_handler/version_handler.h"
#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

class VersionHandlerDispatcher {
 public:
  VersionHandlerDispatcher(routing::Routing& routing);

  template <typename RequestorType>
  void SendGetVersionsResponse(const std::vector<VersionHandler::VersionName>& versions,
                               const RequestorType& requestor, const maidsafe_error& result);

  template <typename RequestorType>
  void SendGetBranchResponse(const std::vector<VersionHandler::VersionName>& versions,
                             const RequestorType& requestor, const maidsafe_error& result);

 private:
  routing::Routing& routing_;
};

template <typename RequestorType>
void VersionHandlerDispatcher::SendGetVersionsResponse(
    const std::vector<VersionHandler::VersionName>&, const RequestorType&,
        const maidsafe_error&) {
  RequestorType::No_generic_handler_is_available__Specialisation_is_required;
}

template <typename RequestorType>
void SendGetBranchResponse(const std::vector<VersionHandler::VersionName>& /*versions*/,
                           const RequestorType& /*requestor*/, const maidsafe_error& /*result*/) {
  RequestorType::No_generic_handler_is_available__Specialisation_is_required;
}

template <>
void VersionHandlerDispatcher::SendGetVersionsResponse(
    const std::vector<VersionHandler::VersionName>& versions,
    const detail::Requestor<nfs::SourcePersona<maidsafe::nfs::Persona::kDataGetter>>& requestor,
    const maidsafe_error& result);

template <>
void VersionHandlerDispatcher::SendGetVersionsResponse(
    const std::vector<VersionHandler::VersionName>& versions,
    const detail::Requestor<nfs::SourcePersona<maidsafe::nfs::Persona::kMaidNode>>& requestor,
    const maidsafe_error& result);

template <>
void VersionHandlerDispatcher::SendGetBranchResponse(
    const std::vector<VersionHandler::VersionName>& versions,
    const detail::Requestor<nfs::SourcePersona<maidsafe::nfs::Persona::kDataGetter>>& requestor,
    const maidsafe_error& result);

template <>
void VersionHandlerDispatcher::SendGetBranchResponse(
    const std::vector<VersionHandler::VersionName>& versions,
    const detail::Requestor<nfs::SourcePersona<maidsafe::nfs::Persona::kMaidNode>>& requestor,
    const maidsafe_error& result);


}

}  // namespace vault

