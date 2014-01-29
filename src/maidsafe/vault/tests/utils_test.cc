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

#include <functional>
#include <memory>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/thread/future.hpp>

#include "maidsafe/common/test.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/passport/types.h"

#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/vault.h"

#include "maidsafe/vault/tests/tests_utils.h"

namespace maidsafe {

namespace vault {

namespace test {

template <int width>
testing::AssertionResult CheckToAndFromFixedWidthString() {
  uint64_t max_value(1);
  for (int pow(1); pow != width + 1; ++pow) {
    max_value *= 256;
    uint64_t reps(std::min(max_value * 2, static_cast<uint64_t>(10000)));
    for (uint64_t i(0); i != reps; ++i) {
      uint32_t input(RandomUint32() % max_value);
      auto fixed_width_string(detail::ToFixedWidthString<width>(input));
      if (static_cast<size_t>(width) != fixed_width_string.size())
        return testing::AssertionFailure() << "Output string size (" << fixed_width_string.size()
                                           << ") != width (" << width << ")";
      uint32_t recovered(detail::FromFixedWidthString<width>(fixed_width_string));
      if (input != recovered)
        return testing::AssertionFailure() << "Recovered value (" << recovered
                                           << ") != initial value (" << input << ")";
    }
  }
  return testing::AssertionSuccess();
}

TEST(UtilsTest, BEH_FixedWidthStringSize1) { CheckToAndFromFixedWidthString<1>(); }

TEST(UtilsTest, BEH_FixedWidthStringSize2) { CheckToAndFromFixedWidthString<2>(); }

TEST(UtilsTest, BEH_FixedWidthStringSize3) { CheckToAndFromFixedWidthString<3>(); }

TEST(UtilsTest, BEH_FixedWidthStringSize4) { CheckToAndFromFixedWidthString<4>(); }

// temp testing

class PublicPmidGetter {
 public :
   PublicPmidGetter();
   void AddEntry(passport::PublicPmid::Name pmid_name,
                 boost::future<passport::PublicPmid>&& future,
                 routing::GivePublicKeyFunctor give_key_functors);
 private:
  typedef std::pair<passport::PublicPmid::Name, routing::GivePublicKeyFunctor> PublicPmidRequest;
  void Poll();
  void Run();
  void CallFunctor(const passport::PublicPmid& public_pmid);
  std::mutex mutex_;
  bool running_;
  std::vector<PublicPmidRequest> requests_;
  std::vector<boost::future<passport::PublicPmid>> futures_, pending_futures_;
  //std::thread thread_;
  std::future<void> worker_future_;
};


// =========================== TEMP test ===================================================

PublicPmidGetter::PublicPmidGetter()
    : mutex_(),
      running_(false),
      requests_(),
      futures_(),
      pending_futures_(),
      worker_future_() {}

void PublicPmidGetter::AddEntry(passport::PublicPmid::Name pmid_name,
                                boost::future<passport::PublicPmid>&& future,
                                routing::GivePublicKeyFunctor give_key) {

  {
    std::lock_guard<std::mutex> lock(mutex_);
    requests_.push_back(PublicPmidRequest(pmid_name, give_key));
    pending_futures_.push_back(std::move(future));
    Run();
  }

}

void PublicPmidGetter::Run() {
  if (!running_) {
      std::cout << std::endl << "starting thread !" << std::endl;
    running_ = true;
    worker_future_ = std::async(std::launch::async, [this]() { this->Poll(); });
  }
}

void PublicPmidGetter::Poll() {
  do {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      std::move(pending_futures_.begin(), pending_futures_.end(),
                std::back_inserter(futures_));
      pending_futures_.resize(0);
      if (futures_.size() == 0U) {
        running_ = false;
        break;
      }
    }
    std::cout << std::endl << " polling on (" << futures_.size() << ") futures" << std::endl;
    auto ready_future_itr = boost::wait_for_any(futures_.begin(), futures_.end());
    //passport::PublicPmid public_pmid;
    //try {
    auto public_pmid = ready_future_itr->get();
    //} catch (...) {
     // FIXME Need to know the pmid name here to call right functor
    //}
    futures_.erase(ready_future_itr);
    CallFunctor(public_pmid);
  } while (true);
}

void PublicPmidGetter::CallFunctor(const passport::PublicPmid &public_pmid) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto itr = std::find_if(requests_.begin(), requests_.end(),
                          [public_pmid](const PublicPmidRequest& request){
    return request.first == public_pmid.name();
  });
  if (itr != requests_.end())
    itr->second(public_pmid.public_key());
  requests_.erase(itr);
}

void RunFutureTestInParallel(int thread_count, std::function<void()> functor) {
  std::vector<std::thread> threads;
  for (int i = 0; i < thread_count; ++i)
    threads.push_back(std::move(std::thread([functor]() { functor(); } )));
  for (auto& thread : threads)
    thread.join();
}

TEST(UtilsTest, BEH_FutureWaitForAny) {
  PublicPmidGetter public_pmid_getter;
  auto pmid(MakePmid());
  passport::PublicPmid::Name pmid_name(PmidName(pmid.name()));
  auto test = [pmid, pmid_name, &public_pmid_getter]() {
    std::vector<boost::promise<passport::PublicPmid>> promises;
    for (int i(0); i < 10 ; ++i) {
      boost::promise<passport::PublicPmid> promise;
      auto future = promise.get_future();
      routing::GivePublicKeyFunctor functor = [i](asymm::PublicKey /*public_key*/) {
        std::cout << std::endl << "called functor" << i << std::endl;
      };
      public_pmid_getter.AddEntry(pmid_name, std::move(future), functor);
      promises.push_back(std::move(promise));
      if (i == 5)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    for (auto& i : promises)
      i.set_value(passport::PublicPmid(pmid));
    };
  RunFutureTestInParallel(10, test);
  std::this_thread::sleep_for(std::chrono::seconds(2));
  RunFutureTestInParallel(2, test);

  // TODO Extend test and fix exception handling mechanism
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
