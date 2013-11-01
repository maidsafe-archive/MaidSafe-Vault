/*  Copyright 2009 MaidSafe.net limited

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

#include "maidsafe/vault/group_db.h"

#include "leveldb/db.h"
#include "leveldb/options.h"

#include "leveldb/status.h"

#include "maidsafe/common/log.h"
#include "maidsafe/common/test.h"
#include "maidsafe/common/utils.h"

#include "maidsafe/passport/types.h"

#include "maidsafe/vault/group_key.h"
#include "maidsafe/vault/maid_manager/maid_manager.h"
#include "maidsafe/vault/maid_manager/metadata.h"
#include "maidsafe/vault/pmid_manager/pmid_manager.h"
#include "maidsafe/vault/version_handler/key.h"
#include "maidsafe/vault/version_handler/value.h"

namespace maidsafe {

namespace vault {

namespace test {

TEST_CASE("GroupDb constructor", "[GroupDb][Unit]") {
  GroupDb<MaidManager> maid_group_db;
  GroupDb<PmidManager> pmid_group_db;
}

TEST_CASE("GroupDb commit", "[GroupDb][Unit]") {
  GroupDb<MaidManager> maid_group_db;
  passport::PublicMaid::Name maid_name(Identity(crypto::Hash<crypto::SHA512>(
                                                    NonEmptyString(RandomAlphaNumericString(36)))));
  CHECK_THROWS_AS(maid_group_db.GetMetadata(maid_name), maidsafe_error);
  maid_group_db.DeleteGroup(maid_name);
  auto metadata = MaidManagerMetadata();
  maid_group_db.AddGroup(maid_name, metadata);
  CHECK(maid_group_db.GetMetadata(maid_name) == metadata);
  CHECK_THROWS_AS(maid_group_db.AddGroup(maid_name, metadata), maidsafe_error);
  maid_group_db.DeleteGroup(maid_name);
  CHECK_THROWS_AS(maid_group_db.GetMetadata(maid_name), maidsafe_error);
}

}  // test

}  // namespace vault

}  // namespace maidsafe
