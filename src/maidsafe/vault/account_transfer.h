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
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "boost/date_time/posix_time/posix_time.hpp"

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
        : request(request_in), group_id(source_in.group_id) {
      request.Merge(request_in, source_in.sender_id);
    }

//     bool HasSender(const routing::GroupSource& source_in) {
//       if (source_in.group_id != group_id)
//         return false;
//       auto itr(std::find(senders.begin(), senders.end(), source_in.sender_id));
//       if (itr == senders.end())
//         return false;
//       return true;
//     }
    void MergePendingRequest(const UnresolvedAccountTransferAction& request_in,
                             const routing::GroupSource& source_in) {
      request.Merge(request_in, source_in.sender_id);
    }
    std::set<routing::SingleId> GetSenders() const {
      return request.GetSenders();
    }
    routing::GroupId GetGroupId() const {
      return group_id;
    }
    UnresolvedAccountTransferAction GetRequest() const {
      return request;
    }

    UnresolvedAccountTransferAction GetResolved(size_t resolve_num) {
      return request.GetResolvedActions(resolve_num);
    }
    bool IsResolved() {
      return request.IsResolved();
    }

   private:
    UnresolvedAccountTransferAction request;
    routing::GroupId group_id;
//     std::set<routing::SingleId> senders;
  };

  AccountTransfer();

  std::unique_ptr<UnresolvedAccountTransferAction> AddUnresolvedAction(
      const UnresolvedAccountTransferAction& request,
      const routing::GroupSource& source,
      AddRequestChecker checker);
  bool CheckHandled(const routing::GroupId& source);

 private:
  AccountTransfer(const AccountTransfer&);
  AccountTransfer& operator=(const AccountTransfer&);
  AccountTransfer(AccountTransfer&&);
  AccountTransfer& operator=(AccountTransfer&&);

  bool RequestExists(const UnresolvedAccountTransferAction& request,
                     const routing::GroupSource& source);
  void CleanUpHandledRequests();

  std::deque<PendingRequest> pending_requests_;
  std::map<routing::GroupId, boost::posix_time::ptime> handled_requests_;
  const size_t kMaxPendingRequestsCount_, kMaxHandledRequestsCount_;
  mutable std::mutex mutex_;
};

// ==================== Implementation =============================================================

template <typename UnresolvedAccountTransferAction>
AccountTransfer<UnresolvedAccountTransferAction>::AccountTransfer()
    : pending_requests_(),
      handled_requests_(),
      kMaxPendingRequestsCount_(100),
      kMaxHandledRequestsCount_(100),
      mutex_() {}

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
  if (CheckHandled(source.group_id)) {
    LOG(kInfo) << "AccountTransfer::AddUnresolvedAction request has been handled";
    return resolved_action;
  }
  std::lock_guard<std::mutex> lock(mutex_);
  if (RequestExists(request, source)) {
    LOG(kVerbose) << "AccountTransfer::AddUnresolvedAction request already existed";
    auto itr(pending_requests_.begin());
    while (itr != pending_requests_.end()) {
      if (itr->GetGroupId() == source.group_id) {
        LOG(kVerbose) << "AccountTransfer::AddUnresolvedAction merge request before having "
                      << itr->GetSenders().size() << " senders";
        itr->MergePendingRequest(request, source);
        LOG(kVerbose) << "AccountTransfer::AddUnresolvedAction merge request after having "
                      << itr->GetSenders().size() << " senders";
        if (checker(itr->GetSenders()) == AddResult::kSuccess) {
          resolved_action.reset(new UnresolvedAccountTransferAction(
              itr->GetResolved(routing::Parameters::group_size / 2)));
          if (itr->IsResolved()) {
            handled_requests_[itr->GetGroupId()] =
                boost::posix_time::microsec_clock::universal_time();
            LOG(kVerbose) << "AccountTransfer::AddUnresolvedAction put "
                          << DebugId(itr->GetGroupId()) << " into handled list";
            if (handled_requests_.size() > kMaxHandledRequestsCount_)
              CleanUpHandledRequests();
            pending_requests_.erase(itr);
          }
        }
        break;
      }
      ++itr;
    }
  } else {
    pending_requests_.push_back(PendingRequest(request, source));
    LOG(kVerbose) << "AccountTransfer::AddUnresolvedAction has " << pending_requests_.size()
                  << " pending requests, allowing " << kMaxPendingRequestsCount_ << " requests";
    if (pending_requests_.size() > kMaxPendingRequestsCount_)
      pending_requests_.pop_front();
  }
  return std::move(resolved_action);
}

template <typename UnresolvedAccountTransferAction>
bool AccountTransfer<UnresolvedAccountTransferAction>::CheckHandled(
    const routing::GroupId& source_group_id) {
  std::lock_guard<std::mutex> lock(mutex_);
  LOG(kVerbose) << "AccountTransfer::CheckHandled handled_requests_.size() "
                << handled_requests_.size();
  auto handled_entry(handled_requests_.find(source_group_id));
  if (handled_entry == handled_requests_.end())
    return false;
  auto cur_time(boost::posix_time::microsec_clock::universal_time());
  if ((cur_time - handled_entry->second).total_seconds() > 60) {
    handled_requests_.erase(handled_entry);
    return false;
  }
  return true;
}

template <typename UnresolvedAccountTransferAction>
bool AccountTransfer<UnresolvedAccountTransferAction>::RequestExists(
    const UnresolvedAccountTransferAction& request, const routing::GroupSource& source) {
  auto request_message_id(request.id);
  for (auto& pending_request : pending_requests_)
    if (request_message_id == pending_request.GetRequest().id) {
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

template <typename UnresolvedAccountTransferAction>
void AccountTransfer<UnresolvedAccountTransferAction>::CleanUpHandledRequests() {
  auto itr(handled_requests_.begin());
  auto cur_time(boost::posix_time::microsec_clock::universal_time());
  while (itr != handled_requests_.end()) {
    if ((cur_time - itr->second).total_seconds() > 60)
      itr = handled_requests_.erase(itr);
    else
      ++itr;
  }
}

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_ACCOUNT_TRANSFER_H_
