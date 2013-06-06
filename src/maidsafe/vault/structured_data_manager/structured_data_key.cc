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
    : data_(data_name_in),
      originator_(originator_in) {}

StructuredDataKey::StructuredDataKey(const std::string& serialised_key) {
  std::string name(serialised_key.substr(0, NodeId::kSize));
  std::string type_as_string(serialised_key.substr(NodeId::kSize, kSuffixWidth_));
  std::string originator(serialised_key.substr(NodeId::kSize + kSuffixWidth_));
  auto type(static_cast<DataTagValue>(detail::FromFixedWidthString<kSuffixWidth_>(type_as_string)));
  return StructuredDataManager::DbKey(GetDataNameVariant(type, Identity(name)),
                                      Identity(originator));
  name_ = GetDataNameVariant(type, Identity(name));
  originator_ = originator;
}

StructuredDataKey::StructuredDataKey() : data_(), originator_() {}

StructuredDataKey::StructuredDataKey(const StructuredDataKey& other)
    : data_(other.data_),
      originator_(other.originator_) {}

StructuredDataKey& StructuredDataKey::operator=(StructuredDataKey other) {
  swap(*this, other);
  return *this;
}

StructuredDataKey::StructuredDataKey(StructuredDataKey&& other)
    : data_(std::move(other.data_)),
      originator_(std::move(other.originator_)) {}

void swap(StructuredDataKey& lhs, StructuredDataKey& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.data_, rhs.data_);
  swap(lhs.originator_, rhs.originator_);
}

std::string StructuredDataKey::Serialise() const {
  static GetTagValueAndIdentityVisitor visitor;
  auto result(boost::apply_visitor(visitor, name_));
  return std::string(result.second.string() +
                    detail::ToFixedWidthString<kSuffixWidth_>(static_cast<uint32_t>(result.first)) +
                    originator_.string());
}

bool operator==(const StructuredDataKey& lhs, const StructuredDataKey& rhs) {
  return std::tie(lhs.data_, lhs.originator_) == std::tie(rhs.data_, rhs.originator_);
}

bool operator!=(const StructuredDataKey& lhs, const StructuredDataKey& rhs) {
  return !operator==(lhs, rhs);
}

bool operator<(const StructuredDataKey& lhs, const StructuredDataKey& rhs) {
  return std::tie(lhs.data_, lhs.originator_) < std::tie(rhs.data_, rhs.originator_);
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
