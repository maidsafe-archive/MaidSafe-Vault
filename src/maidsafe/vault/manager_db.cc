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
#include "maidsafe/vault/manager_db.h"

#include <utility>


#include "maidsafe/common/types.h"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/nfs/types.h"
#include "maidsafe/vault/types.h"

namespace maidsafe {

namespace vault {

template<>
std::string ManagerDb<StructuredDataManager>::SerialiseKey(
    const typename StructuredDataManager::DbKey& key) const {
  auto result(boost::apply_visitor(GetTagValueAndIdentityVisitor(), key.data_name));
  return std::string(result.second.string() +
                     detail::ToFixedWidthString<kSuffixWidth_>(static_cast<int32_t>(result.first)) +
                     key.originator.string());
}

template<>
typename StructuredDataManager::DbKey ManagerDb<StructuredDataManager>::ParseKey(
    const std::string& serialised_key) const {
  std::string name(serialised_key.substr(0, NodeId::kSize));
  std::string type_as_string(serialised_key.substr(NodeId::kSize, kSuffixWidth_));
  std::string originator(serialised_key.substr(NodeId::kSize + kSuffixWidth_));
  auto type(static_cast<DataTagValue>(detail::FromFixedWidthString<kSuffixWidth_>(type_as_string)));
  return StructuredDataManager::DbKey(GetDataNameVariant(type, Identity(name)),
                                      Identity(originator));
}

}  // namespace vault

}  // namespace maidsafe
