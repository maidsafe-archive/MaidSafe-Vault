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

#include <string>

#include "maidsafe/vault/utils.h"

namespace maidsafe {

namespace vault {

namespace detail {

// bool NodeRangeCheck(maidsafe::routing::Routing& routing, const NodeId& node_id) {
//   return routing.IsNodeIdInGroupRange(node_id);  // provisional call to Is..
// }

void ExtractElementsFromFilename(const std::string& filename,
                                 std::string& hash,
                                 size_t& file_number) {
  auto it(std::find(filename.begin(), filename.end(), '.'));
  if (it == filename.end()) {
    LOG(kError) << "No dot in the file name.";
    throw std::exception();
  }
  file_number = static_cast<size_t>(std::stoi(std::string(filename.begin(), it)));
  hash = std::string(it + 1, filename.end());
}

boost::filesystem::path GetFilePath(const boost::filesystem::path& base_path,
                                    const std::string& hash,
                                    size_t file_number) {
  return base_path / (std::to_string(file_number) + "." + hash);
}

bool MatchingDiskElements(const protobuf::DiskStoredElement& lhs,
                          const protobuf::DiskStoredElement& rhs) {
  return lhs.data_name() == rhs.data_name() &&
         lhs.version() == rhs.version() &&
         (rhs.serialised_value().empty() ?
             true : lhs.serialised_value() == rhs.serialised_value());
}

}  // namespace detail

}  // namespace vault

}  // namespace maidsafe
