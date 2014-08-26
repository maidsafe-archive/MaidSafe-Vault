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



#include <algorithm>

#include "maidsafe/common/test.h"
#include "maidsafe/vault/tests/tests_utils.h"
#include "maidsafe/vault/tests/vault_network.h"

namespace maidsafe {

namespace vault {

namespace test {

typedef StructuredDataVersions::VersionName VersionName;

class VersionHandlerTest : public testing::Test {
 public:
  VersionHandlerTest() : env_(VaultEnvironment::g_environment()) {}

  std::vector<VaultNetwork::ClientPtr>& GetClients() { return env_->clients_; }

  template <typename DataName>
  std::vector<VersionName> GetVersions(const DataName& data_name) {
    try {
      auto future(env_->clients_.front()->GetVersions(data_name));
      return future.get();
    }
    catch (const maidsafe_error&) {
      throw;
    }
  }

 protected:
  std::shared_ptr<VaultNetwork> env_;
};


TEST_F(VersionHandlerTest, FUNC_CreateVersionTree) {
  ImmutableData chunk(NonEmptyString(RandomAlphaNumericString(1024)));
  StructuredDataVersions::VersionName v_aaa(0, ImmutableData::Name(Identity(std::string(64, 'a'))));
  auto create_version_future(GetClients().front()->CreateVersionTree(chunk.name(), v_aaa, 10, 20));
  EXPECT_NO_THROW(create_version_future.get());
  try {
    auto future(GetClients().front()->GetVersions(chunk.name()));
    auto versions(future.get());
    EXPECT_EQ(versions.front().id, v_aaa.id);
  }
  catch (const maidsafe_error& error) {
    EXPECT_TRUE(false) << "Failed to retrieve version: " << boost::diagnostic_information(error);
  }
  LOG(kVerbose) << "Version Created";
  Sleep(std::chrono::seconds(5));
}

TEST_F(VersionHandlerTest, FUNC_FailingPut) {
  ImmutableData chunk(NonEmptyString(RandomAlphaNumericString(1024)));
  StructuredDataVersions::VersionName v_aaa(0, ImmutableData::Name(Identity(std::string(64, 'a'))));
  auto put_version_future(
      GetClients().front()->PutVersion(chunk.name(), StructuredDataVersions::VersionName(), v_aaa));
  EXPECT_THROW(put_version_future.get(), maidsafe_error) << "should have failed";
  Sleep(std::chrono::seconds(5));
}

TEST_F(VersionHandlerTest, FUNC_CreateGet) {
  ImmutableData chunk(NonEmptyString(RandomAlphaNumericString(1024)));
  StructuredDataVersions::VersionName v_aaa(0, ImmutableData::Name(Identity(std::string(64, 'a'))));
  auto create_version_future(GetClients().front()->CreateVersionTree(chunk.name(), v_aaa, 10, 20));
  EXPECT_NO_THROW(create_version_future.get()) << "failure to create version";
  Sleep(std::chrono::seconds(3));
  try {
    auto future(GetClients().front()->GetVersions(chunk.name()));
    auto versions(future.get());
    EXPECT_EQ(versions.front().id, v_aaa.id);
  }
  catch (const maidsafe_error& error) {
    EXPECT_TRUE(false) << "Failed to retrieve version: " << boost::diagnostic_information(error);
  }
  Sleep(std::chrono::seconds(5));
}

TEST_F(VersionHandlerTest, FUNC_PutGet) {
  ImmutableData chunk(NonEmptyString(RandomAlphaNumericString(1024)));
  StructuredDataVersions::VersionName v_aaa(0, ImmutableData::Name(Identity(std::string(64, 'a'))));
  StructuredDataVersions::VersionName v_bbb(0, ImmutableData::Name(Identity(std::string(64, 'b'))));
  auto create_version_future(GetClients().front()->CreateVersionTree(chunk.name(), v_aaa, 10, 20));
  EXPECT_NO_THROW(create_version_future.get()) << "failure to create version";
  Sleep(std::chrono::seconds(3));
  auto put_version_future(GetClients().front()->PutVersion(chunk.name(), v_aaa, v_bbb));
  EXPECT_NO_THROW(put_version_future.get()) << "failure to put version";
  Sleep(std::chrono::seconds(2));
  LOG(kVerbose) << "Put Version Succeeded";
  try {
    auto future(GetClients().front()->GetVersions(chunk.name()));
    auto versions(future.get());
    EXPECT_EQ(versions.front().id, v_bbb.id);
  }
  catch (const maidsafe_error& error) {
    EXPECT_TRUE(false) << "Failed to retrieve version: " << boost::diagnostic_information(error);
  }
}

TEST_F(VersionHandlerTest, FUNC_DeleteBranchUntilFork) {
  ImmutableData::Name name(Identity(RandomAlphaNumericString(64)));
  VersionName v0_aaa(0, ImmutableData::Name(Identity(std::string(64, 'a'))));
  VersionName v1_bbb(1, ImmutableData::Name(Identity(std::string(64, 'b'))));
  VersionName v2_ccc(2, ImmutableData::Name(Identity(std::string(64, 'c'))));
  VersionName v2_ddd(2, ImmutableData::Name(Identity(std::string(64, 'd'))));
  VersionName v3_fff(3, ImmutableData::Name(Identity(std::string(64, 'f'))));
  VersionName v4_iii(4, ImmutableData::Name(Identity(std::string(64, 'i'))));

  std::vector<std::pair<VersionName, VersionName>> puts;
  puts.push_back(std::make_pair(v0_aaa, v1_bbb));
  puts.push_back(std::make_pair(v1_bbb, v2_ccc));
  puts.push_back(std::make_pair(v2_ccc, v3_fff));
  puts.push_back(std::make_pair(v1_bbb, v2_ddd));
  puts.push_back(std::make_pair(v3_fff, v4_iii));

  auto create_version_future(GetClients().front()->CreateVersionTree(name, v0_aaa, 10, 20));
  EXPECT_NO_THROW(create_version_future.get());

  for (const auto& put : puts) {
    auto put_version_future(GetClients().front()->PutVersion(name, put.first, put.second));
    EXPECT_NO_THROW(put_version_future.get());
  }

  try {
    auto versions(GetVersions(name));
    for (const auto& version : versions)
      LOG(kVerbose) << version.id->string();
    EXPECT_NE(std::find(std::begin(versions), std::end(versions), v4_iii), std::end(versions));
    EXPECT_NE(std::find(std::begin(versions), std::end(versions), v2_ddd), std::end(versions));
  }
  catch (const std::exception& error) {
    EXPECT_TRUE(false) << "Version should have existed " << boost::diagnostic_information(error);
  }

  EXPECT_NO_THROW(GetClients().front()->DeleteBranchUntilFork(name, v4_iii));
  Sleep(std::chrono::seconds(4));
  LOG(kVerbose) << "After delete";

  try {
    auto versions(GetVersions(name));
    LOG(kVerbose) << "versions.size: " << versions.size();
    for (const auto& version : versions)
      LOG(kVerbose) << version.id->string();
    EXPECT_EQ(std::find(std::begin(versions), std::end(versions), v4_iii), std::end(versions));
    EXPECT_NE(std::find(std::begin(versions), std::end(versions), v2_ddd), std::end(versions));
  }
  catch (const std::exception& error) {
    EXPECT_TRUE(false) << boost::diagnostic_information(error);
  }
  Sleep(std::chrono::seconds(5));
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
