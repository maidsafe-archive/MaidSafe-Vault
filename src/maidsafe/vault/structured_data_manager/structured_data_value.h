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

#ifndef MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_VALUE_H_
#define MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_VALUE_H_

#include "boost/optional.hpp"

#include "maidsafe/nfs/message.h"
#include "maidsafe/routing/api_config.h"
#include "maidsafe/data_types/structured_data_versions.h"


namespace maidsafe {

namespace vault {

struct StructuredDataValue {
  StructuredDataValue();
  StructuredDataValue(const StructuredDataValue& other);
  StructuredDataValue& operator=(StructuredDataValue other);
  // TODO(Team) when we get std::optional we can have move ctr but boost prevents this just now
  nfs::MessageId entry_id;
  boost::optional<StructuredDataVersions::VersionName> version;
  boost::optional<StructuredDataVersions::VersionName> new_version;
  boost::optional<routing::ReplyFunctor> reply_functor;
  boost::optional<StructuredDataVersions::serialised_type> serialised_db_value;  // account xfer
};

// no move in boost::optional uncomment when std::optional is available
// void swap(const StructuredDataValue& lhs, const StructuredDataValue& rhs) MAIDSAFE_NOEXCEPT;

bool operator==(const StructuredDataValue& lhs, const StructuredDataValue& rhs);

bool operator!=(const StructuredDataValue& lhs, const StructuredDataValue& rhs);

bool operator<(const StructuredDataValue& lhs, const StructuredDataValue& rhs);

bool operator>(const StructuredDataValue& lhs, const StructuredDataValue& rhs);

bool operator<=(const StructuredDataValue& lhs, const StructuredDataValue& rhs);

bool operator>=(const StructuredDataValue& lhs, const StructuredDataValue& rhs);



struct StructuredDataDbValue {
  explicit StructuredDataDbValue(const StructuredDataValue& structured_data_value);
  StructuredDataDbValue(const StructuredDataDbValue& other);
  StructuredDataDbValue(StructuredDataDbValue&& other);
  StructuredDataDbValue& operator=(StructuredDataDbValue other);
  boost::optional<StructuredDataVersions::VersionName> version;
  boost::optional<StructuredDataVersions::VersionName> new_version;
  boost::optional<StructuredDataVersions::serialised_type> serialised_db_value;  // account xfer
};

// no move in boost::optional uncomment when std::optional is available
// void swap(const StructuredDataDbValue& lhs, const StructuredDataDbValue& rhs) MAIDSAFE_NOEXCEPT;

bool operator==(const StructuredDataDbValue& lhs, const StructuredDataDbValue& rhs);

bool operator!=(const StructuredDataDbValue& lhs, const StructuredDataDbValue& rhs);

bool operator<(const StructuredDataDbValue& lhs, const StructuredDataDbValue& rhs);

bool operator>(const StructuredDataDbValue& lhs, const StructuredDataDbValue& rhs);

bool operator<=(const StructuredDataDbValue& lhs, const StructuredDataDbValue& rhs);

bool operator>=(const StructuredDataDbValue& lhs, const StructuredDataDbValue& rhs);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_VALUE_H_
