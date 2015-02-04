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
#include <algorithm>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/thread/future.hpp"

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

TEST(UtilsTest, BEH_MedianOfNoValues) {
  std::vector<size_t> values;
  EXPECT_THROW(Median(values), std::exception);
}

TEST(UtilsTest, BEH_MedianOfOneValue) {
  std::vector<size_t> values;
  size_t value(RandomUint32());
  values.push_back(value);
  EXPECT_EQ(value, Median(values));
}

TEST(UtilsTest, BEH_MedianOfTwoValues) {
  std::vector<size_t> values;
  size_t value1(RandomUint32()), value2(RandomUint32());
  values.push_back(value1);
  values.push_back(value2);
  EXPECT_EQ((value1 / 2) + (value2 / 2) + (value1 & value2 & 1), Median(values));
}

TEST(UtilsTest, BEH_MedianOfOddNumberOfValues) {
  std::vector<size_t> values;
  for (size_t i = 0; i != 21; ++i)
    values.push_back(i);
  std::random_shuffle(values.begin(), values.end());
  EXPECT_EQ(10, Median(values));
}

TEST(UtilsTest, BEH_MedianOfEvenNumberOfValues) {
  std::vector<size_t> values;
  for (size_t i = 0; i != 20; ++i)
    values.push_back(i);
  std::random_shuffle(values.begin(), values.end());
  EXPECT_EQ(9, Median(values));
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
