/***************************************************************************************************
 *  Copyright 2013 MaidSafe.net limited                                                            *
 *                                                                                                 *
 *  The following source code is property of MaidSafe.net limited and is not meant for external    *
 *  use.  The use of this code is governed by the licence file licence.txt found in the root of    *
 *  this directory and also on www.maidsafe.net.                                                   *
 *                                                                                                 *
 *  You are not free to copy, amend or otherwise use this source code without the explicit         *
 *  written permission of the board of directors of MaidSafe.net.                                  *
 **************************************************************************************************/

#include "maidsafe/vault/db_key.h"

#include "maidsafe/vault/utils.h"


namespace maidsafe {

namespace vault {

const int DbKey::kPaddedWidth_(1);

DbKey::DbKey(const DataNameVariant& name) : name_(name) {}

DbKey::DbKey() : name_() {}

DbKey::DbKey(const std::string& serialised_key) : name_() {
  std::string name(serialised_key.substr(0, NodeId::kSize));
  std::string type_as_string(serialised_key.substr(NodeId::kSize, kPaddedWidth_));
  auto type(static_cast<DataTagValue>(detail::FromFixedWidthString<kPaddedWidth_>(type_as_string)));
  name_ = GetDataNameVariant(type, Identity(name));
}

DbKey::DbKey(const DbKey& other) : name_(other.name_)  {}

DbKey::DbKey(DbKey&& other) : name_(std::move(other.name_)) {}

DbKey& DbKey::operator=(DbKey other) {
  swap(*this, other);
  return *this;
}

std::string DbKey::Serialise() const {
  static GetTagValueAndIdentityVisitor visitor;
  auto result(boost::apply_visitor(visitor, name_));
  return std::string(result.second.string() +
            detail::ToFixedWidthString<kPaddedWidth_>(static_cast<uint32_t>(result.first)));
}


void swap(DbKey& lhs, DbKey& rhs) MAIDSAFE_NOEXCEPT {
  using std::swap;
  swap(lhs.name_, rhs.name_);
}

bool operator==(const DbKey& lhs, const DbKey& rhs) {
  return lhs.name_ == rhs.name_;
}

bool operator!=(const DbKey& lhs, const DbKey& rhs) {
  return !operator==(lhs, rhs);
}

bool operator<(const DbKey& lhs, const DbKey& rhs) {
  return lhs.name_ < rhs.name_;
}

bool operator>(const DbKey& lhs, const DbKey& rhs) {
  return operator<(rhs, lhs);
}

bool operator<=(const DbKey& lhs, const DbKey& rhs) {
  return !operator>(lhs, rhs);
}

bool operator>=(const DbKey& lhs, const DbKey& rhs) {
  return !operator<(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
