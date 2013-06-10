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

#include "maidsafe/vault/utils.h"


namespace maidsafe {

namespace vault {

const int StructuredDataKey::kPaddedWidth_(1);

StructuredDataKey::StructuredDataKey() : data_name_(), originator_() {}

StructuredDataKey::StructuredDataKey(const DataNameVariant& data_name,
                                     const Identity& originator)
    : data_name_(data_name),
      originator_(originator) {}

StructuredDataKey::StructuredDataKey(const std::string& serialised_key)
    : data_name_(),
      originator_() {
  std::string name(serialised_key.substr(0, NodeId::kSize));
  std::string type_as_string(serialised_key.substr(NodeId::kSize, kPaddedWidth_));
  auto type(static_cast<DataTagValue>(detail::FromFixedWidthString<kPaddedWidth_>(type_as_string)));
  data_name_ = GetDataNameVariant(type, Identity(name));
  originator_ = Identity(serialised_key.substr(NodeId::kSize + kPaddedWidth_));
}

StructuredDataKey::StructuredDataKey(const StructuredDataKey& other)
    : data_name_(other.data_name_),
      originator_(other.originator_) {}

StructuredDataKey& StructuredDataKey::operator=(StructuredDataKey other) {
  swap(*this, other);
  return *this;
}

StructuredDataKey::StructuredDataKey(StructuredDataKey&& other)
    : data_name_(std::move(other.data_name_)),
      originator_(std::move(other.originator_)) {}

void swap(StructuredDataKey& lhs, StructuredDataKey& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.data_name_, rhs.data_name_);
  swap(lhs.originator_, rhs.originator_);
}

std::string StructuredDataKey::Serialise() const {
  static GetTagValueAndIdentityVisitor visitor;
  auto result(boost::apply_visitor(visitor, data_name_));
  return std::string(
      result.second.string() +
      detail::ToFixedWidthString<kPaddedWidth_>(static_cast<uint32_t>(result.first)) +
      originator_.string());
}

bool operator==(const StructuredDataKey& lhs, const StructuredDataKey& rhs) {
  return std::tie(lhs.data_name_, lhs.originator_) == std::tie(rhs.data_name_, rhs.originator_);
}

bool operator!=(const StructuredDataKey& lhs, const StructuredDataKey& rhs) {
  return !operator==(lhs, rhs);
}

bool operator<(const StructuredDataKey& lhs, const StructuredDataKey& rhs) {
  return std::tie(lhs.data_name_, lhs.originator_) < std::tie(rhs.data_name_, rhs.originator_);
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
