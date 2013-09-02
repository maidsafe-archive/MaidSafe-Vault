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

bool ShouldRetry(routing::Routing& routing, const NodeId& source_id, const NodeId& data_name) {
  return routing.network_status() >= Parameters::kMinNetworkHealth &&
         routing.EstimateInGroup(source_id, data_name);
}

/* Commented by Mahmoud on 2 Sep -- It may be of no use any more or needs refactoring
void SendReply(const nfs::Message& original_message,
               const maidsafe_error& return_code,
               const routing::ReplyFunctor& reply_functor) {
  if (!reply_functor)
    return;
  nfs::Reply reply(CommonErrors::success);
  if (return_code.code() != CommonErrors::success)
    reply = nfs::Reply(return_code, original_message.Serialise().data);
  reply_functor(reply.Serialise()->string());
} */

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

// To be moved to Routing
bool operator ==(const routing::GroupSource& lhs,  const routing::GroupSource& rhs) {
  return lhs.group_id == rhs.group_id &&
         lhs.sender_id == rhs.sender_id;
}

}  // namespace vault

}  // namespace maidsafe
