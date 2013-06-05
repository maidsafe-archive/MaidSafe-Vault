/***************************************************************************************************
 *  Copyright 2013 maidsafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use. The use of this code is governed by the licence file licence.txt found in the root of     *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit written *
 *  permission of the board of directors of MaidSafe.net.                                          *
 **************************************************************************************************/

#include "maidsafe/vault/structured_data_manager/structured_data_key.h"

#include <algorithm>
#include <tuple>

namespace maidsafe {

namespace vault {

StructuredDataKey::StructuredDataKey(const DataNameVariant& data_name_in,
                                     const Identity& originator_in)
    : data_name(data_name_in),
      originator(originator_in) {}

StructuredDataKey::StructuredDataKey() : data_name(), originator() {}

StructuredDataKey::StructuredDataKey(const StructuredDataKey& other)
    : data_name(other.data_name),
      originator(other.originator) {}

StructuredDataKey& StructuredDataKey::operator=(StructuredDataKey other) {
  swap(*this, other);
  return *this;
}

StructuredDataKey::StructuredDataKey(StructuredDataKey&& other)
    : data_name(std::move(other.data_name)),
      originator(std::move(other.originator)) {}

void swap(StructuredDataKey& lhs, StructuredDataKey& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.data_name, rhs.data_name);
  swap(lhs.originator, rhs.originator);
}

bool operator==(const StructuredDataKey& lhs, const StructuredDataKey& rhs) {
  return std::tie(lhs.data_name, lhs.originator) == std::tie(rhs.data_name, rhs.originator);
}

bool operator!=(const StructuredDataKey& lhs, const StructuredDataKey& rhs) {
  return !operator==(lhs, rhs);
}

bool operator<(const StructuredDataKey& lhs, const StructuredDataKey& rhs) {
  return std::tie(lhs.data_name, lhs.originator) < std::tie(rhs.data_name, rhs.originator);
}

bool operator>(const StructuredDataKey& lhs, const StructuredDataKey& rhs) {
  return operator<(rhs, lhs);
}

bool operator<=(const StructuredDataKey& lhs, const StructuredDataKey& rhs) {
  return !operator>(lhs, rhs);
}

bool operator>=(const StructuredDataKey& lhs, const StructuredDataKey& rhs) {
  return !operator<(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
