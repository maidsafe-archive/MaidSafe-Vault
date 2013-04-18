/***************************************************************************************************
 *  Copyright 2012 MaidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of MaidSafe.net.                                  *
 **************************************************************************************************/

#include "maidsafe/vault/vault.h"

#include <functional>
#include <memory>

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include "maidsafe/common/test.h"
#include "maidsafe/common/utils.h"
#include "maidsafe/vault/utils.h"

#include "maidsafe/passport/types.h"


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
                              this_id,
                              target_id))
    proximity = routing::GroupRangeStatus::kInRange;
  else if (holders.size() <= routing::Parameters::closest_nodes_size ||
      !NodeId::CloserToTarget(holders.at(routing::Parameters::closest_nodes_size - 1),
                              this_id,
                              target_id))
   proximity = routing::GroupRangeStatus::kInProximalRange;
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

}  // namespace test

}  // namespace vault

}  // namespace maidsafe
