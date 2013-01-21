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

#ifndef MAIDSAFE_VAULT_UTILS_H_
#define MAIDSAFE_VAULT_UTILS_H_

#include <string>

#include "maidsafe/routing/routing_api.h"

#include "maidsafe/vault/disk_based_storage_messages_pb.h"

namespace maidsafe {

namespace vault {

namespace detail {

inline bool NodeRangeCheck(routing::Routing& routing, const NodeId& node_id) {
  return routing.IsNodeIdInGroupRange(node_id);
}

void ExtractElementsFromFilename(const std::string& filename,
                                 std::string& hash,
                                 size_t& file_number);

boost::filesystem::path GetFilePath(const boost::filesystem::path& base_path,
                                    const std::string& hash,
                                    size_t file_number);

bool MatchingDiskElements(const protobuf::DiskStoredElement& lhs,
                          const protobuf::DiskStoredElement& rhs);

}  // namespace detail

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_UTILS_H_
