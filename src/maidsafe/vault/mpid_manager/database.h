/*  Copyright 2015 MaidSafe.net limited

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

#ifndef MAIDSAFE_VAULT_MPID_MANAGER_DATABASE_H_
#define MAIDSAFE_VAULT_MPID_MANAGER_DATABASE_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "boost/multi_index_container.hpp"
#include "boost/multi_index/global_fun.hpp"
#include "boost/multi_index/member.hpp"
#include "boost/multi_index/ordered_index.hpp"
#include "boost/multi_index/identity.hpp"

#include "maidsafe/common/data_types/immutable_data.h"
#include "maidsafe/common/identity.h"
#include "maidsafe/passport/types.h"

namespace maidsafe {

namespace vault {

using GroupName = Identity;
using MessageKey = Identity;
using GKPair = std::pair<GroupName, MessageKey>;
using DbTransferInfo = std::map<Identity, std::vector<GKPair>>;

struct DatabaseEntry {
  DatabaseEntry(const MessageKey& key_in,
                const uint32_t size_in,
                const GroupName& mpid_in)
      : key(key_in), size(size_in), mpid(mpid_in) {}
  DatabaseEntry Key() const { return *this; }
  MessageKey key;
  uint32_t size;
  GroupName mpid;
};

struct EntryKey_Tag {};
struct EntryMpid_Tag {};

typedef boost::multi_index_container<
    DatabaseEntry,
    boost::multi_index::indexed_by<
        boost::multi_index::ordered_unique<boost::multi_index::tag<EntryKey_Tag>,
            BOOST_MULTI_INDEX_MEMBER(DatabaseEntry, MessageKey, key)>,
        boost::multi_index::ordered_non_unique<boost::multi_index::tag<EntryMpid_Tag>,
            BOOST_MULTI_INDEX_MEMBER(DatabaseEntry, GroupName, mpid)>
    >
> DatabaseEntrySet;

using EntryByKey = boost::multi_index::index<DatabaseEntrySet, EntryKey_Tag>::type;
using EntryByMpid = boost::multi_index::index<DatabaseEntrySet, EntryMpid_Tag>::type;
using EntryByMpidIterator = DatabaseEntrySet::index<EntryMpid_Tag>::type::iterator;

class MpidManagerDatabase {
 public:
  MpidManagerDatabase();

  void Put(const MessageKey& key, const uint32_t size, const GroupName& mpid);
  void Delete(const MessageKey& key);
  bool Has(const MessageKey& key);

  bool HasGroup(const GroupName& mpid);
  MessageKey GetAccountChunkName(const GroupName& mpid);
  std::pair<uint32_t, uint32_t> GetStatistic(const GroupName& mpid);
  std::vector<MessageKey> GetEntriesForMPID(const GroupName& mpid);

//  DbTransferInfo GetTransferInfo(std::shared_ptr<routing::CloseNodesChange> close_nodes_change);

 private:
  void DeleteGroup(const GroupName& mpid);
//  void PutIntoTransferInfo(const NodeId& new_holder,
//                           const GroupName& mpid,
//                           const MessageKey& key,
//                           DbTransferInfo& transfer_info);

  DatabaseEntrySet container_;
  mutable std::mutex mutex_;
};

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_MPID_MANAGER_DATABASE_H_

