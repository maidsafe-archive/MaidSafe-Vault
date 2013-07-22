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

#include <functional>
#include <memory>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "maidsafe/common/test.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/passport/types.h"

#include "maidsafe/vault/utils.h"
#include "maidsafe/vault/vault.h"


namespace maidsafe {

namespace vault {

namespace test {


template<int width>
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

TEST(UtilsTest, BEH_FixedWidthStringSize1) {
  CheckToAndFromFixedWidthString<1>();
}

TEST(UtilsTest, BEH_FixedWidthStringSize2) {
  CheckToAndFromFixedWidthString<2>();
}

TEST(UtilsTest, BEH_FixedWidthStringSize3) {
  CheckToAndFromFixedWidthString<3>();
}

TEST(UtilsTest, BEH_FixedWidthStringSize4) {
  CheckToAndFromFixedWidthString<4>();
}

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
