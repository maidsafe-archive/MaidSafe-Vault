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


#include "boost/variant.hpp"

#include "maidsafe/vault/operation_handlers.h"
#include "maidsafe/vault/pmid_node/service.h"

namespace maidsafe {

namespace vault {

namespace detail {

template <>
template <>
void OperationHandler<
         typename ValidateSenderType<GetPmidAccountResponseFromPmidManagerToPmidNode>::type,
         Accumulator<PmidNodeServiceMessages>,
         typename Accumulator<PmidNodeServiceMessages>::AddCheckerFunctor,
         PmidNodeService>::operator()(
    const GetPmidAccountResponseFromPmidManagerToPmidNode& message,
    const GetPmidAccountResponseFromPmidManagerToPmidNode::Sender& sender,
    const GetPmidAccountResponseFromPmidManagerToPmidNode::Receiver& /*receiver*/) {
  if (!validate_sender(message, sender))
    return;
  {
    std::lock_guard<std::mutex> lock(mutex);
    if (accumulator.CheckHandled(message))
      return;
    auto result(accumulator.AddPendingRequest(message, sender, checker));
    if (result == Accumulator<PmidNodeServiceMessages>::AddResult::kSuccess) {
      int failures(0);
      auto responses(accumulator.Get(message));
      std::vector<std::set<nfs_vault::DataName>> response_vec;
      for (const auto& response : responses) {
        auto typed_response(boost::get<GetPmidAccountResponseFromPmidManagerToPmidNode>(response));
        if (typed_response.contents->return_code.value.code() == CommonErrors::success)
          response_vec.push_back(typed_response.contents->names);
        else
         failures++;
      }
      service->HandlePmidAccountResponses(response_vec, failures);
    } else if (result == Accumulator<PmidNodeServiceMessages>::AddResult::kFailure) {
      service->StartUp();
    }
  }
}

}  // namespace detail

}  // namespace vault

}  // namespace maidsafe
