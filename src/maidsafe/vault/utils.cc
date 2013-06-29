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

#include "maidsafe/vault/utils.h"

#include <string>

#include "boost/filesystem/operations.hpp"
#include "leveldb/status.h"

#include "maidsafe/common/types.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/vault/parameters.h"


namespace fs = boost::filesystem;

namespace maidsafe {

namespace vault {

namespace detail {

void InitialiseDirectory(const boost::filesystem::path& directory) {
  if (fs::exists(directory)) {
    if (!fs::is_directory(directory))
      ThrowError(CommonErrors::not_a_directory);
  } else {
    fs::create_directory(directory);
  }
}

bool ShouldRetry(routing::Routing& routing, const nfs::Message& message) {
  return routing.network_status() >= Parameters::kMinNetworkHealth &&
         routing.EstimateInGroup(message.source().node_id, NodeId(message.data().name.string()));
}

void SendReply(const nfs::Message& original_message,
               const maidsafe_error& return_code,
               const routing::ReplyFunctor& reply_functor) {
  if (!reply_functor)
    return;
  nfs::Reply reply(CommonErrors::success);
  if (return_code.code() != CommonErrors::success)
    reply = nfs::Reply(return_code, original_message.Serialise().data);
  reply_functor(reply.Serialise()->string());
}

template<>
std::string ToFixedWidthString<1>(uint32_t number) {
  assert(number < 256);
  return std::string(1, static_cast<char>(number));
}

template<>
uint32_t FromFixedWidthString<1>(const std::string& number_as_string) {
  assert(number_as_string.size() == 1U);
  return static_cast<uint32_t>(static_cast<unsigned char>(number_as_string[0]));
}

}  // namespace detail



CheckHoldersResult CheckHolders(const routing::MatrixChange& matrix_change,
                                const NodeId& this_id,
                                const NodeId& target) {
  CheckHoldersResult holders_result;
  std::vector<NodeId> old_matrix(matrix_change.old_matrix),
                      new_matrix(matrix_change.new_matrix);
  const auto comparator([&target](const NodeId& lhs, const NodeId& rhs) {
                            return NodeId::CloserToTarget(lhs, rhs, target);
                        });
  std::sort(old_matrix.begin(), old_matrix.end(), comparator);
  std::sort(new_matrix.begin(), new_matrix.end(), comparator);
  std::set_difference(new_matrix.begin(),
                      new_matrix.end(),
                      old_matrix.begin(),
                      old_matrix.end(),
                      std::back_inserter(holders_result.new_holders),
                      comparator);
  std::set_difference(old_matrix.begin(),
                      old_matrix.end(),
                      new_matrix.begin(),
                      new_matrix.end(),
                      std::back_inserter(holders_result.old_holders),
                      comparator);

  holders_result.proximity_status = routing::GroupRangeStatus::kOutwithRange;
  if (new_matrix.size() <= routing::Parameters::node_group_size ||
      !NodeId::CloserToTarget(new_matrix.at(routing::Parameters::node_group_size - 1),
                              this_id,
                              target)) {
    holders_result.proximity_status = routing::GroupRangeStatus::kInRange;
  } else if (new_matrix.size() <= routing::Parameters::closest_nodes_size ||
             !NodeId::CloserToTarget(new_matrix.at(routing::Parameters::closest_nodes_size - 1),
                                     this_id, target)) {
    holders_result.proximity_status = routing::GroupRangeStatus::kInProximalRange;
  }
  return holders_result;
}

template<>
typename VersionManager::DbKey
    GetKeyFromMessage<VersionManager>(const nfs::Message& message) {
  if (!message.data().type)
    ThrowError(CommonErrors::parsing_error);
  return StructuredDataKey(GetDataNameVariant(*message.data().type, message.data().name),
                           message.data().originator);
}

template<>
typename DataManager::RecordName GetRecordName<DataManager>(
    const typename DataManager::DbKey& db_key) {
  return db_key;
}

template<>
typename VersionManager::RecordName GetRecordName<VersionManager>(
    const typename VersionManager::DbKey& db_key) {
  return db_key.data_name();
}

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
