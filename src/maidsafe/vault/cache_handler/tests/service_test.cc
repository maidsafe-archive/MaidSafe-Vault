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

#include "boost/filesystem.hpp"

#include "maidsafe/common/test.h"
#include "maidsafe/common/asio_service.h"
#include "maidsafe/passport/passport.h"
#include "maidsafe/routing/routing_api.h"

#include "maidsafe/vault/cache_handler/service.h"
#include "maidsafe/vault/tests/tests_utils.h"


namespace maidsafe {

namespace vault {

namespace test {

class CacheHandlerServiceTest : public testing::Test {
 public:
  CacheHandlerServiceTest()
      : kTestRoot_(maidsafe::test::CreateTestPath("MaidSafe_Test_Vault")),
        vault_root_dir_(*kTestRoot_ / RandomAlphaNumericString(8)),
        routing_(passport::CreatePmidAndSigner().first),
        cache_handler_service_(routing_, vault_root_dir_),
        asio_service_(2) {
    boost::filesystem::create_directory(vault_root_dir_);
  }

  template <typename Data>
  void Store(const Data& data) {
    cache_handler_service_.CacheStore(data, is_long_term_cacheable<Data>());
  }

  template <typename Data>
  boost::optional<Data> Get(const typename Data::Name& data_name) {
    auto data(cache_handler_service_.CacheGet<Data>(data_name, is_long_term_cacheable<Data>()));
    if (!data)
      BOOST_THROW_EXCEPTION(MakeError(CommonErrors::no_such_element));
    return data;
  }

 protected:
  const maidsafe::test::TestPath kTestRoot_;
  boost::filesystem::path vault_root_dir_;
  routing::Routing routing_;
  CacheHandlerService cache_handler_service_;
  AsioService asio_service_;
};

TEST_F(CacheHandlerServiceTest, BEH_ShortTermPutGet) {
  passport::Anmaid anmaid;
  passport::PublicAnmaid public_anmaid(anmaid);
  EXPECT_ANY_THROW(Get<passport::PublicAnmaid>(public_anmaid.name()));
  Store(public_anmaid);
  EXPECT_NO_THROW(Get<passport::PublicAnmaid>(public_anmaid.name()));
}

TEST_F(CacheHandlerServiceTest, BEH_LongTermPutGet) {
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  EXPECT_ANY_THROW(Get<ImmutableData>(data.name()));
  Store(data);
  EXPECT_NO_THROW(Get<ImmutableData>(data.name()));
}


TEST_F(CacheHandlerServiceTest, BEH_GetCachedResponseFromCacheHandlerToDataGetter) {
  routing::SingleId maid_node((NodeId(NodeId::IdType::kRandomId)));
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  nfs_client::DataNameAndContentOrReturnCode content(data);
  auto cached_response(CreateMessage<nfs::GetCachedResponseFromCacheHandlerToDataGetter>(content));
  routing::SingleSource source((NodeId(NodeId::IdType::kRandomId)));
  EXPECT_TRUE(cache_handler_service_.HandleMessage(cached_response, source, maid_node));
  EXPECT_NO_THROW(Get<ImmutableData>(data.name()));
}

TEST_F(CacheHandlerServiceTest, BEH_GetResponseFromDataManagerToDataGetter) {
  routing::SingleId maid_node((NodeId(NodeId::IdType::kRandomId)));
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  nfs_client::DataNameAndContentOrReturnCode content(data);
  auto response(CreateMessage<nfs::GetResponseFromDataManagerToDataGetter>(content));
  auto group_source(CreateGroupSource(data.name()));
  EXPECT_TRUE(cache_handler_service_.HandleMessage(response, *group_source.begin(), maid_node));
  EXPECT_NO_THROW(Get<ImmutableData>(data.name()));
}

TEST_F(CacheHandlerServiceTest, BEH_GetResponseFromDataManagerToMaidNode) {
  routing::SingleId maid_node((NodeId(NodeId::IdType::kRandomId)));
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  nfs_client::DataNameAndContentOrReturnCode content(data);
  auto response(CreateMessage<nfs::GetResponseFromDataManagerToMaidNode>(content));
  auto group_source(CreateGroupSource(data.name()));
  EXPECT_TRUE(cache_handler_service_.HandleMessage(response, *group_source.begin(), maid_node));
  EXPECT_NO_THROW(Get<ImmutableData>(data.name()));
}

TEST_F(CacheHandlerServiceTest, BEH_GetCachedResponseFromCacheHandlerToMaidNode) {
  routing::SingleId maid_node((NodeId(NodeId::IdType::kRandomId)));
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  nfs_client::DataNameAndContentOrReturnCode content(data);
  auto cached_response(CreateMessage<nfs::GetCachedResponseFromCacheHandlerToMaidNode>(content));
  routing::SingleSource source((NodeId(NodeId::IdType::kRandomId)));
  EXPECT_TRUE(cache_handler_service_.HandleMessage(cached_response, source, maid_node));
  EXPECT_NO_THROW(Get<ImmutableData>(data.name()));
}

TEST_F(CacheHandlerServiceTest, BEH_GetRequestFromMaidNodeToDataManager) {
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  routing::SingleSource source_node((NodeId(NodeId::IdType::kRandomId)));
  routing::GroupId group_id(NodeId(data.name()->string()));
  nfs_vault::DataName content(data.name());
  auto get_request(CreateMessage<nfs::GetRequestFromMaidNodeToDataManager>(content));
  EXPECT_FALSE(cache_handler_service_.HandleMessage(get_request, source_node, group_id));
  Store(data);
  EXPECT_TRUE(cache_handler_service_.HandleMessage(get_request, source_node, group_id));
}

TEST_F(CacheHandlerServiceTest, BEH_GetRequestFromDataGetterToDataManager) {
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  routing::SingleSource source_node((NodeId(NodeId::IdType::kRandomId)));
  routing::GroupId group_id(NodeId(data.name()->string()));
  nfs_vault::DataName content(data.name());
  auto get_request(CreateMessage<nfs::GetRequestFromDataGetterToDataManager>(content));
  EXPECT_FALSE(cache_handler_service_.HandleMessage(get_request, source_node, group_id));
  Store(data);
  EXPECT_TRUE(cache_handler_service_.HandleMessage(get_request, source_node, group_id));
}

}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
