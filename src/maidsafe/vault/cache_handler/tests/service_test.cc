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
    use of the MaidSafe Software.
*/

#include "maidsafe/common/test.h"
#include "maidsafe/common/asio_service.h"

#include "maidsafe/routing/routing_api.h"

#include "maidsafe/vault/cache_handler/service.h"
#include "maidsafe/vault/tests/tests_utils.h"


namespace maidsafe {

namespace vault {

namespace test {


class CacheHandlerServiceTest {
 public:
  CacheHandlerServiceTest() :
      pmid_(MakePmid()),
      kTestRoot_(maidsafe::test::CreateTestPath("MaidSafe_Test_Vault")),
      vault_root_dir_(*kTestRoot_ / RandomAlphaNumericString(8)),
      routing_(pmid_),
      cache_handler_service_(routing_, vault_root_dir_),
      asio_service_(2) {
    boost::filesystem::create_directory(vault_root_dir_);
    asio_service_.Start();
  }

  template <typename Data>
  void Store(const Data& data) {
    cache_handler_service_.CacheStore(data, is_long_term_cacheable<Data>());
  }

  template <typename Data>
  boost::optional<Data> Get(const typename Data::Name& data_name) {
    auto data(cache_handler_service_.CacheGet<Data>(data_name, is_long_term_cacheable<Data>()));
    if (!data)
      ThrowError(CommonErrors::no_such_element);
    return data;
  }

 protected:
  passport::Pmid pmid_;
  const maidsafe::test::TestPath kTestRoot_;
  boost::filesystem::path vault_root_dir_;
  routing::Routing routing_;
  CacheHandlerService cache_handler_service_;
  AsioService asio_service_;
};

TEST_CASE_METHOD(CacheHandlerServiceTest, "short term cache put/get", "[ShortTermCachePutGet]") {
  passport::Anmaid anmaid;
  passport::PublicAnmaid public_anmaid(anmaid);
  REQUIRE_THROWS(Get<passport::PublicAnmaid>(public_anmaid.name()));
  Store(public_anmaid);
  REQUIRE_NOTHROW(Get<passport::PublicAnmaid>(public_anmaid.name()));
}

TEST_CASE_METHOD(CacheHandlerServiceTest, "long term cache put/get", "[LongTermCachePutGet]") {
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  REQUIRE_THROWS(Get<ImmutableData>(data.name()));
  Store(data);
  REQUIRE_NOTHROW(Get<ImmutableData>(data.name()));
}

TEST_CASE_METHOD(CacheHandlerServiceTest, "operations storing to cache", "[PutInCache]") {
  routing::SingleId maid_node((NodeId(NodeId::kRandomId)));
  ImmutableData data(NonEmptyString(RandomString(kTestChunkSize)));
  nfs_client::DataNameAndContentOrReturnCode content(data);

  SECTION("GetCachedResponseFromCacheHandlerToDataGetter") {
    auto cached_response(CreateMessage<nfs::GetCachedResponseFromCacheHandlerToDataGetter>(content));
    routing::SingleSource source((NodeId(NodeId::kRandomId)));
    CHECK(cache_handler_service_.HandleMessage(cached_response, source, maid_node));
    REQUIRE_NOTHROW(Get<ImmutableData>(data.name()));
  }

  SECTION("GetResponseFromDataManagerToDataGetter") {
    auto response(CreateMessage<nfs::GetResponseFromDataManagerToDataGetter>(content));
    auto group_source(CreateGroupSource(data.name()));
    CHECK(cache_handler_service_.HandleMessage(response, *group_source.begin(), maid_node));
    REQUIRE_NOTHROW(Get<ImmutableData>(data.name()));
  }

  SECTION("GetResponseFromDataManagerToMaidNode") {
    auto response(CreateMessage<nfs::GetResponseFromDataManagerToMaidNode>(content));
    auto group_source(CreateGroupSource(data.name()));
    CHECK(cache_handler_service_.HandleMessage(response, *group_source.begin(), maid_node));
    REQUIRE_NOTHROW(Get<ImmutableData>(data.name()));
  }

  SECTION("GetCachedResponseFromCacheHandlerToMaidNode") {
    auto cached_response(CreateMessage<nfs::GetCachedResponseFromCacheHandlerToMaidNode>(content));
    routing::SingleSource source((NodeId(NodeId::kRandomId)));
    CHECK(cache_handler_service_.HandleMessage(cached_response, source, maid_node));
    REQUIRE_NOTHROW(Get<ImmutableData>(data.name()));
  }
}

}  //  namespace test

}  //  namespace vault

}  //  namespace maidsafe
