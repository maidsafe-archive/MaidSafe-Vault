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

#include "maidsafe/vault/public_pmid_helper.h"

#include <functional>
#include <memory>

#include <boost/thread/future.hpp>

#include "maidsafe/common/test.h"

#include "maidsafe/passport/types.h"
#include "maidsafe/routing/routing_api.h"

#include "maidsafe/vault/types.h"

#include "maidsafe/vault/tests/tests_utils.h"

namespace maidsafe {

namespace vault {

namespace test {

void RunFutureTestInParallel(int thread_count, std::function<void()> functor) {
  std::vector<std::thread> threads;
  for (int i = 0; i < thread_count; ++i)
    threads.push_back(std::move(std::thread([functor]() { functor(); } )));
  for (auto& thread : threads)
    thread.join();
}

TEST(PublicPmidHelperTest, BEH_FutureWaitForAny) {
  detail::PublicPmidHelper public_pmid_helper;
  auto pmid(MakePmid());
  passport::PublicPmid::Name pmid_name(PmidName(pmid.name()));
  auto test = [pmid, pmid_name, &public_pmid_helper]() {
    std::vector<boost::promise<passport::PublicPmid>> promises;
    for (int i(0); i < 10 ; ++i) {
      boost::promise<passport::PublicPmid> promise;
      auto future = promise.get_future();
      routing::GivePublicKeyFunctor functor = [i, pmid](asymm::PublicKey public_key) {
        ASSERT_TRUE(rsa::MatchingKeys(public_key, pmid.public_key()));
        std::cout << std::endl << "called functor" << i << std::endl;
      };
      public_pmid_helper.AddEntry(std::move(future), functor);
      promises.push_back(std::move(promise));
      if (i == 5)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    for (auto& i : promises)
      i.set_value(passport::PublicPmid(pmid));
    };
  RunFutureTestInParallel(10, test);
  std::this_thread::sleep_for(std::chrono::seconds(1));
  RunFutureTestInParallel(2, test);

  // TODO Extend test and fix exception handling mechanism
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
