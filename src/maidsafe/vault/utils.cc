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

#include "maidsafe/vault/utils.h"

#include <string>

#include "boost/filesystem/operations.hpp"
#include "leveldb/status.h"

#include "maidsafe/common/types.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/vault/parameters.h"
#include "maidsafe/vault/pmid_node/service.h"

#include "maidsafe/vault/operations_visitor.h"

namespace fs = boost::filesystem;

namespace maidsafe {

namespace vault {

namespace detail {

template <typename T>
DataNameVariant GetNameVariant(const T&) {
  T::invalid_parameter;
  return DataNameVariant();
}

template <>
DataNameVariant GetNameVariant(const nfs_vault::DataName& data) {
  return GetDataNameVariant(data.type, data.raw_name);
}

template <>
DataNameVariant GetNameVariant(const nfs_vault::DataNameAndContent& data) {
  return GetNameVariant(data.name);
}

template <>
DataNameVariant GetNameVariant(const nfs_vault::DataAndPmidHint& data) {
  return GetNameVariant(data.data.name);
}

template <>
DataNameVariant GetNameVariant(const nfs_client::DataAndReturnCode& data) {
  return GetNameVariant(data.data.name);
}

template <>
DataNameVariant GetNameVariant(const nfs_client::DataNameAndContentOrReturnCode& data) {
  return data.data ? GetNameVariant(data.data->name) :
                     GetNameVariant(data.data_name_and_return_code->name);
}

template <>
DataNameVariant GetNameVariant(const nfs_vault::DataNameAndContentOrCheckResult& data) {
  return GetNameVariant(data.name);
}

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
      service->SendAccountRequest();
    }
  }
}

void InitialiseDirectory(const boost::filesystem::path& directory) {
  if (fs::exists(directory)) {
    if (!fs::is_directory(directory))
      ThrowError(CommonErrors::not_a_directory);
  } else {
    fs::create_directory(directory);
  }
}

bool ShouldRetry(routing::Routing& routing, const NodeId& source_id, const NodeId& data_name) {
  return routing.network_status() >= Parameters::kMinNetworkHealth &&
         routing.EstimateInGroup(source_id, data_name);
}

}  // namespace detail

std::unique_ptr<leveldb::DB> InitialiseLevelDb(const boost::filesystem::path& db_path) {
  if (boost::filesystem::exists(db_path))
    boost::filesystem::remove_all(db_path);
  leveldb::DB* db(nullptr);
  leveldb::Options options;
  options.create_if_missing = true;
  options.error_if_exists = true;
  leveldb::Status status(leveldb::DB::Open(options, db_path.string(), &db));
  if (!status.ok())
    ThrowError(CommonErrors::filesystem_io_error);
  assert(db);
  return std::move(std::unique_ptr<leveldb::DB>(db));
}

}  // namespace vault

}  // namespace maidsafe
