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

#ifndef MAIDSAFE_VAULT_ACCOUNT_TRANSFER_H_
#define MAIDSAFE_VAULT_ACCOUNT_TRANSFER_H_

#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include "maidsafe/common/node_id.h"

#include "maidsafe/routing/parameters.h"

#include "maidsafe/vault/unresolved_account_transfer_action.h"

namespace maidsafe {

namespace vault {

template <typename UnresolvedAccountTransferAction>
class AccountTransfer {
 public:
  enum class AddResult {
    kSuccess,
    kWaiting,
    kFailure,
    kHandled
  };

  class AddRequestChecker {
   public:
    explicit AddRequestChecker(size_t required_requests)
        : required_requests_(required_requests) {
      assert((required_requests <= routing::Parameters::group_size) &&
             "Invalid number of requests");
    }

    AddResult operator()(const std::set<routing::SingleId>& requests) {
      if (requests.size() < required_requests_) {
        LOG(kInfo) << "AccountTransfer::AddRequestChecke::operator() not enough pending requests";
        return AddResult::kWaiting;
      }
      return AddResult::kSuccess;
    }

   private:
    size_t required_requests_;
  };

  struct PendingRequest {
   public:
    PendingRequest(const UnresolvedAccountTransferAction& request_in,
                   const routing::GroupSource& source_in)
        : request(request_in), group_id(source_in.group_id), senders() {
      senders.insert(source_in.sender_id);
    }
    bool HasSender(const routing::GroupSource& source_in) {
      if (source_in.group_id != group_id)
        return false;
      auto itr(std::find(senders.begin(), senders.end(), source_in.sender_id));
      if (itr == senders.end())
        return false;
      return true;
    }
    void MergePendingRequest(const UnresolvedAccountTransferAction& request_in,
                             const routing::GroupSource& source_in) {
      senders.insert(source_in.sender_id);
      request.Merge(request_in);
    }
    std::set<routing::SingleId> GetSenders() const {
      return senders;
    }
    routing::GroupId GetGroupId() const {
      return group_id;
    }
    UnresolvedAccountTransferAction Getrequest() const {
      return request;
    }
   private:
    UnresolvedAccountTransferAction request;
    routing::GroupId group_id;
    std::set<routing::SingleId> senders;
  };

  AccountTransfer();

  std::unique_ptr<UnresolvedAccountTransferAction> AddUnresolvedAction(
      const UnresolvedAccountTransferAction& request,
      const routing::GroupSource& source,
      AddRequestChecker checker);
  bool CheckHandled(const routing::GroupSource& source);

 private:
  AccountTransfer(const AccountTransfer&);
  AccountTransfer& operator=(const AccountTransfer&);
  AccountTransfer(AccountTransfer&&);
  AccountTransfer& operator=(AccountTransfer&&);

  bool RequestExists(const UnresolvedAccountTransferAction& request,
                     const routing::GroupSource& source);

  std::deque<PendingRequest> pending_requests_;
  std::deque<routing::GroupId> handled_requests_;
  const size_t kMaxPendingRequestsCount_, kMaxHandledRequestsCount_;
};

// ==================== Implementation =============================================================

template <typename UnresolvedAccountTransferAction>
AccountTransfer<UnresolvedAccountTransferAction>::AccountTransfer()
    : pending_requests_(),
      handled_requests_(),
      kMaxPendingRequestsCount_(100),
      kMaxHandledRequestsCount_(100) {}

template <typename UnresolvedAccountTransferAction>
std::unique_ptr<UnresolvedAccountTransferAction>
    AccountTransfer<UnresolvedAccountTransferAction>::AddUnresolvedAction(
        const UnresolvedAccountTransferAction& request,
        const routing::GroupSource& source,
        AddRequestChecker checker) {
  LOG(kVerbose) << "AccountTransfer::AddUnresolvedAction for GroupSource "
                << HexSubstr(source.group_id.data.string()) << " sent from "
                << HexSubstr(source.sender_id->string());
  std::unique_ptr<UnresolvedAccountTransferAction> resolved_action;
  auto itr(pending_requests_.begin());
  if (CheckHandled(source)) {
    LOG(kInfo) << "AccountTransfer::AddUnresolvedAction request has been handled";
    return resolved_action;
  }

  if (!RequestExists(request, source)) {
    pending_requests_.push_back(PendingRequest(request, source));
    LOG(kVerbose) << "AccountTransfer::AddUnresolvedAction has " << pending_requests_.size()
                  << " pending requests, allowing " << kMaxPendingRequestsCount_ << " requests";
    if (pending_requests_.size() > kMaxPendingRequestsCount_)
      pending_requests_.pop_front();
  } else {
    LOG(kInfo) << "AccountTransfer::AddUnresolvedAction request already existed";
    while (itr != pending_requests_.end()) {
      if (itr->GetGroupId() == source.group_id) {
        itr->MergePendingRequest(request, source);
        if (checker(itr->GetSenders()) == AddResult::kSuccess) {
          resolved_action.reset(new UnresolvedAccountTransferAction(itr->Getrequest()));
          handled_requests_.push_back(itr->GetGroupId());
          pending_requests_.erase(itr);
        }
        break;
      }
      ++itr;
    }
  }
  return std::move(resolved_action);
}

template <typename UnresolvedAccountTransferAction>
bool AccountTransfer<UnresolvedAccountTransferAction>::CheckHandled(
    const routing::GroupSource& source) {
  for (auto& group_id : handled_requests_)
    if (group_id == source.group_id)
      return true;
  return false;
}

template <typename UnresolvedAccountTransferAction>
bool AccountTransfer<UnresolvedAccountTransferAction>::RequestExists(
    const UnresolvedAccountTransferAction& request, const routing::GroupSource& source) {
  auto request_message_id(request.id);
  for (auto& pending_request : pending_requests_)
    if (request_message_id == pending_request.Getrequest().id) {
      LOG(kWarning) << "AccountTransfer::RequestExists,  reguest with message id "
                    << request_message_id.data
                    << " with group_id " << HexSubstr(source.group_id->string())
                    << " already exists in the pending requests list";
      return true;
    }
  LOG(kInfo) << "AccountTransfer::RequestExists,  reguest with message id "
             << request_message_id.data
             << " with group_id " << HexSubstr(source.group_id->string())
             << " doesn't exists in the pending requests list";
  return false;
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_ACCOUNT_TRANSFER_H_
