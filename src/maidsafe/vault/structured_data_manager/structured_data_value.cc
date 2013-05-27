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
#include "maidsafe/vault/structured_data_manager/structured_data_value.h"

#include "boost/optional.hpp"
#include "maidsafe/data_types/data_name_variant.h"
#include "maidsafe/data_types/structured_data_versions.h"
#include "maidsafe/nfs/persona_id.h"
#include "maidsafe/vault/types.h"

namespace maidsafe {

namespace vault {

StructuredDataValue::StructuredDataValue() :
  version(), new_version(), reply_functor(), serialised_db_value() {}

StructuredDataValue::StructuredDataValue(const StructuredDataValue& other) :
  version(other.version),
  new_version(other.new_version),
  reply_functor(other.reply_functor),
  serialised_db_value(other.serialised_db_value) {}

StructuredDataValue& StructuredDataValue::operator=(StructuredDataValue other) {
  version = other.version;
  new_version = other.new_version;
  reply_functor = other.reply_functor;
  serialised_db_value = other.serialised_db_value;
  // TODO (dirvine) need to check that we dhould not parse and
  // compare serialised StructuredDataVersions
  return *this;
}


// no move in boost::optional uncomment when std::optional is available
//void swap(const StructuredDataValue& lhs,
//          const StructuredDataValue& rhs) MAIDSAFE_NOEXCEPT {
//  using std::swap;
//  swap(lhs.version, rhs.version);
//  swap(lhs.new_version, rhs.new_version);
//  swap(lhs.reply_functor, rhs.reply_functor);
//  swap(lhs.serialised_db_value, rhs.serialised_db_value);
//}

bool operator==(const StructuredDataValue& lhs,
                const StructuredDataValue& rhs) {
  return lhs.version == rhs.version &&
  lhs.new_version == rhs.new_version &&
  lhs.serialised_db_value == rhs.serialised_db_value;
}

bool operator!=(const StructuredDataValue& lhs,
                const StructuredDataValue& rhs) {
  return !operator==(lhs, rhs);
}

bool operator<(const StructuredDataValue& lhs,
               const StructuredDataValue& rhs) {
  return std::tie(lhs.version, lhs.new_version, lhs.serialised_db_value) <
         std::tie(rhs.version, rhs.new_version, rhs.serialised_db_value);
}
bool operator>(const StructuredDataValue& lhs,
               const StructuredDataValue& rhs) {
  return operator<(rhs, lhs);
}

bool operator<=(const StructuredDataValue& lhs,
                const StructuredDataValue& rhs) {
  return !operator>(lhs, rhs);
}

bool operator>=(const StructuredDataValue& lhs,
                const StructuredDataValue& rhs) {
  return !operator<(lhs, rhs);
}


StructuredDataDbValue::StructuredDataDbValue(const StructuredDataValue& structured_data_value) :
  version(structured_data_value.version),
  new_version(structured_data_value.new_version),
  serialised_db_value(structured_data_value.serialised_db_value) {}

StructuredDataDbValue::StructuredDataDbValue(const StructuredDataDbValue& other) :
  version(other.version),
  new_version(other.new_version),
  serialised_db_value(other.serialised_db_value) {}

StructuredDataDbValue&  StructuredDataDbValue::operator=(StructuredDataDbValue other) {
  version = other.version;
  new_version = other.new_version;
  serialised_db_value = other.serialised_db_value;
  return *this;
}

// no move in boost::optional uncomment when std::optional is available
//void swap(const StructuredDataValue& lhs,
//  void swap(const StructuredDataDbValue& lhs,
//            const StructuredDataDbValue& rhs) MAIDSAFE_NOEXCEPT {
//  using std::swap;
//  swap(lhs.version, rhs.version);
//  swap(lhs.new_version, rhs.new_version);
//  swap(lhs.serialised_db_value, rhs.serialised_db_value);
//}

  bool operator==(const StructuredDataDbValue& lhs,
                  const StructuredDataDbValue& rhs) {
  return lhs.version == rhs.version &&
  lhs.new_version == rhs.new_version &&
  lhs.serialised_db_value == rhs.serialised_db_value;
  // TODO (dirvine) need to check that we dhould not parse and
  // compare serialised StructuredDataVersions
}

  bool operator!=(const StructuredDataDbValue& lhs,
                  const StructuredDataDbValue& rhs) {
  return !operator==(lhs, rhs);
}

  bool operator<(const StructuredDataDbValue& lhs,
                 const StructuredDataDbValue& rhs) {
  return lhs.version < rhs.version &&
  lhs.new_version < rhs.new_version &&
  lhs.serialised_db_value < rhs.serialised_db_value;
  // TODO (dirvine) need to check that we dhould not parse and
  // compare serialised StructuredDataVersions
}
  bool operator>(const StructuredDataDbValue& lhs,
                 const StructuredDataDbValue& rhs) {
  return operator<(rhs, lhs);
}

  bool operator<=(const StructuredDataDbValue& lhs,
                  const StructuredDataDbValue& rhs) {
  return !operator>(lhs, rhs);
}

  bool operator>=(const StructuredDataDbValue& lhs,
                  const StructuredDataDbValue& rhs) {
  return !operator<(lhs, rhs);
}


}  // namespace vault

}  // namespace maidsafe
