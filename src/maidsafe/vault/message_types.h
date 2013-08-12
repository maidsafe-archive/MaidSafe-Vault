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

#ifndef MAIDSAFE_VAULT_MESSAGE_TYPES_H_
#define MAIDSAFE_VAULT_MESSAGE_TYPES_H_

#include "maidsafe/routing/message.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/nfs/message_wrapper.h"


namespace maidsafe {

namespace vault {

namespace maid_manager {

//typedef nfs::MessageWrapper<nfs::MessageAction::kPutRequest,
//                            nfs::DestinationPersona<nfs::Persona::kMaidManager>,
//                            nfs::SourcePersona<nfs::Persona::kMaidNode>> MaidNodePut;
//
//typedef nfs::MessageWrapper<nfs::MessageAction::kDeleteRequest,
//                            nfs::DestinationPersona<nfs::Persona::kMaidManager>,
//                            nfs::SourcePersona<nfs::Persona::kMaidNode>> MaidNodeDelete;

}  // namespace maid_manager


namespace data_manager {

//typedef nfs::MessageWrapper<nfs::MessageAction::kGetRequest,
//                            nfs::DestinationPersona<nfs::Persona::kDataManager>,
//                            nfs::SourcePersona<nfs::Persona::kMaidNode>> MaidNodeGet;
//
//typedef nfs::MessageWrapper<nfs::MessageAction::kGetRequest,
//                            nfs::DestinationPersona<nfs::Persona::kDataManager>,
//                            nfs::SourcePersona<nfs::Persona::kPmidNode>> PmidNodeGet;
//
//typedef nfs::MessageWrapper<nfs::MessageAction::kPutRequest,
//                            nfs::DestinationPersona<nfs::Persona::kDataManager>,
//                            nfs::SourcePersona<nfs::Persona::kMaidManager>> MaidManagerPut;
//
//typedef nfs::MessageWrapper<nfs::MessageAction::kDeleteRequest,
//                            nfs::DestinationPersona<nfs::Persona::kDataManager>,
//                            nfs::SourcePersona<nfs::Persona::kMaidManager>> MaidManagerDelete;

}  // namespace data_manager

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MESSAGE_TYPES_H_
