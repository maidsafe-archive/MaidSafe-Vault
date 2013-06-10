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
#include "maidsafe/vault/structured_data_manager/structured_data_unresolved_entry_value.h"

#include <tuple>


namespace maidsafe {

namespace vault {

StructuredDataUnresolvedEntryValue::StructuredDataUnresolvedEntryValue()
    : version(),
      new_version(),
      reply_functor(),
      serialised_db_value() {}

StructuredDataUnresolvedEntryValue::StructuredDataUnresolvedEntryValue(
    const StructuredDataUnresolvedEntryValue& other)
        : version(other.version),
          new_version(other.new_version),
          reply_functor(other.reply_functor),
          serialised_db_value(other.serialised_db_value) {}

StructuredDataUnresolvedEntryValue& StructuredDataUnresolvedEntryValue::operator=(
    StructuredDataUnresolvedEntryValue other) {
  version = other.version;
  new_version = other.new_version;
  reply_functor = other.reply_functor;
  serialised_db_value = other.serialised_db_value;
  // TODO(dirvine) need to check that we should not parse and
  // compare serialised StructuredDataVersions
  return *this;
}

// no move in boost::optional uncomment when std::optional is available
// void swap(const StructuredDataUnresolvedEntryValue& lhs,
//           const StructuredDataUnresolvedEntryValue& rhs) MAIDSAFE_NOEXCEPT {
//  using std::swap;
//  swap(lhs.version, rhs.version);
//  swap(lhs.new_version, rhs.new_version);
//  swap(lhs.reply_functor, rhs.reply_functor);
//  swap(lhs.serialised_db_value, rhs.serialised_db_value);
// }

bool operator==(const StructuredDataUnresolvedEntryValue& lhs,
                const StructuredDataUnresolvedEntryValue& rhs) {
  return lhs.version == rhs.version &&
         lhs.new_version == rhs.new_version &&
         lhs.serialised_db_value == rhs.serialised_db_value;
}

bool operator!=(const StructuredDataUnresolvedEntryValue& lhs,
                const StructuredDataUnresolvedEntryValue& rhs) {
  return !operator==(lhs, rhs);
}

bool operator<(const StructuredDataUnresolvedEntryValue& lhs,
               const StructuredDataUnresolvedEntryValue& rhs) {
  return std::tie(lhs.version, lhs.new_version, lhs.serialised_db_value) <
         std::tie(rhs.version, rhs.new_version, rhs.serialised_db_value);
}

bool operator>(const StructuredDataUnresolvedEntryValue& lhs,
               const StructuredDataUnresolvedEntryValue& rhs) {
  return operator<(rhs, lhs);
}

bool operator<=(const StructuredDataUnresolvedEntryValue& lhs,
                const StructuredDataUnresolvedEntryValue& rhs) {
  return !operator>(lhs, rhs);
}

bool operator>=(const StructuredDataUnresolvedEntryValue& lhs,
                const StructuredDataUnresolvedEntryValue& rhs) {
  return !operator<(lhs, rhs);
}

}  // namespace vault

}  // namespace maidsafe
