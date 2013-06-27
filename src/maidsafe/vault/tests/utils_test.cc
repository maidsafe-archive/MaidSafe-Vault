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

routing::GroupRangeStatus GetStatus(std::vector<NodeId> holders,
                                    NodeId this_id,
                                    NodeId target_id) {
  routing::GroupRangeStatus proximity;
  std::sort(holders.begin(),
            holders.end(),
            [target_id] (const NodeId& lhs, const NodeId& rhs) {
              return NodeId::CloserToTarget(lhs, rhs, target_id);
            });
  proximity = routing::GroupRangeStatus::kOutwithRange;
  if (holders.size() <= routing::Parameters::node_group_size ||
      !NodeId::CloserToTarget(holders.at(routing::Parameters::node_group_size - 1),
                              this_id, target_id)) {
    proximity = routing::GroupRangeStatus::kInRange;
  } else if (holders.size() <= routing::Parameters::closest_nodes_size ||
             !NodeId::CloserToTarget(holders.at(routing::Parameters::closest_nodes_size - 1),
                                     this_id, target_id)) {
    proximity = routing::GroupRangeStatus::kInProximalRange;
  }
  return proximity;
}

TEST(UtilsTest, BEH_CheckHolders) {
    NodeId this_id(NodeId::kRandomId), target_id(NodeId::kRandomId);
  routing::MatrixChange matrix_change;
  auto result(CheckHolders(matrix_change, this_id, target_id));
  EXPECT_EQ(result.new_holders.size(), 0);
  EXPECT_EQ(result.old_holders.size(), 0);
  EXPECT_EQ(GetStatus(matrix_change.new_matrix, this_id, target_id),
            result.proximity_status);

  for (size_t index(0); index < 10; ++index)
    matrix_change.old_matrix.push_back(NodeId(NodeId::kRandomId));
  result = CheckHolders(matrix_change, this_id, target_id);
  EXPECT_EQ(result.new_holders.size(), 0);
  EXPECT_EQ(result.old_holders.size(), 10);
  EXPECT_EQ(GetStatus(matrix_change.new_matrix, this_id, target_id),
            result.proximity_status);

  matrix_change.old_matrix.clear();
  for (size_t index(0); index < 10; ++index)
    matrix_change.new_matrix.push_back(NodeId(NodeId::kRandomId));
  result = CheckHolders(matrix_change, this_id, target_id);
  EXPECT_EQ(result.new_holders.size(), 10);
  EXPECT_EQ(result.old_holders.size(), 0);
  EXPECT_EQ(GetStatus(matrix_change.new_matrix, this_id, target_id),
            result.proximity_status);

  for (auto node : matrix_change.new_matrix)
    matrix_change.old_matrix.push_back(node);

  result = CheckHolders(matrix_change, this_id, target_id);
  EXPECT_EQ(result.new_holders.size(), 0);
  EXPECT_EQ(result.old_holders.size(), 0);
  EXPECT_EQ(GetStatus(matrix_change.new_matrix, this_id, target_id),
            result.proximity_status);

  std::vector<NodeId> difference;
  for (size_t index(0); index < 5; ++index)
    difference.push_back(NodeId(NodeId::kRandomId));
  matrix_change.old_matrix.insert(matrix_change.old_matrix.begin(),
                                  difference.begin(),
                                  difference.end());
  result = CheckHolders(matrix_change, this_id, target_id);
  EXPECT_EQ(result.new_holders.size(), 0);
  EXPECT_EQ(result.old_holders.size(), 5);
  for (size_t index(0); index < 5; ++index)
    EXPECT_NE(std::find(difference.begin(),
                        difference.end(),
                        result.old_holders.at(index)), difference.end());
  EXPECT_EQ(GetStatus(matrix_change.new_matrix, this_id, target_id),
            result.proximity_status);

  matrix_change.old_matrix = matrix_change.new_matrix;
  matrix_change.new_matrix.insert(matrix_change.new_matrix.begin(),
                                  difference.begin(),
                                  difference.end());
  result = CheckHolders(matrix_change, this_id, target_id);
  EXPECT_EQ(result.new_holders.size(), 5);
  EXPECT_EQ(result.old_holders.size(), 0);
  for (size_t index(0); index < 5; ++index)
    EXPECT_NE(std::find(difference.begin(),
                        difference.end(),
                        result.new_holders.at(index)), difference.end());
  EXPECT_EQ(GetStatus(matrix_change.new_matrix, this_id, target_id),
            result.proximity_status);

  std::vector<NodeId> another_difference;
  for (size_t index(0); index < 5; ++index)
    another_difference.push_back(NodeId(NodeId::kRandomId));
  matrix_change.old_matrix.insert(matrix_change.old_matrix.begin(),
                                  another_difference.begin(),
                                  another_difference.end());
  result = CheckHolders(matrix_change, this_id, target_id);
  EXPECT_EQ(result.new_holders.size(), 5);
  EXPECT_EQ(result.old_holders.size(), 5);
  for (size_t index(0); index < 5; ++index)
    EXPECT_NE(std::find(difference.begin(),
                        difference.end(),
                        result.new_holders.at(index)), difference.end());
  for (size_t index(0); index < 5; ++index)
    EXPECT_NE(std::find(another_difference.begin(),
                        another_difference.end(),
                        result.old_holders.at(index)), another_difference.end());
  EXPECT_EQ(GetStatus(matrix_change.new_matrix, this_id, target_id),
            result.proximity_status);
}

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
