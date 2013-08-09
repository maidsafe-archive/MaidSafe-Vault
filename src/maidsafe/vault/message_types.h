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

#include "maidsafe/nfs/types.h"

namespace maidsafe {

namespace vault {

namespace maid_manager {


typedef MessageWrapper<MessageAction::kPutRequest,
                       DestinationPersona<Persona::kMaidManager>,
                       SourcePersona<Persona::kMaidNode>> MaidNodePut;
typedef MessageWrapper<MessageAction::kDeleteRequest,
                       DestinationPersona<Persona::kMaidManager>,
                       SourcePersona<Persona::kMaidNode>> MaidNodeDelete;
}  // namespace maid_manager


namespace data_manager {

typedef MessageWrapper<MessageAction::kGetRequest,
                       DestinationPersona<Persona::kDataManager>,
                       SourcePersona<Persona::kMaidNode>> MaidNodeGet;
typedef MessageWrapper<MessageAction::kGetRequest,
                       DestinationPersona<Persona::kDataManager>,
                       SourcePersona<Persona::kPmidNode>> PmidNodeGet;
typedef MessageWrapper<MessageAction::kPutRequest,
                       DestinationPersona<Persona::kDataManager>,
                       SourcePersona<Persona::kMaidManager>> MaidManagerPut;
typedef MessageWrapper<MessageAction::kDeleteRequest,
                       DestinationPersona<Persona::kDataManager>,
                       SourcePersona<Persona::kMaidManager>> MaidManagerDelete;
}  // namespace data_manager

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MESSAGE_TYPES_H_
