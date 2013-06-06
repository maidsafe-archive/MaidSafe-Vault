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

#ifndef MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_UNRESOLVED_ENTRY_VALUE_H_
#define MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_UNRESOLVED_ENTRY_VALUE_H_

#include "boost/optional.hpp"

#include "maidsafe/routing/api_config.h"
#include "maidsafe/data_types/structured_data_versions.h"


namespace maidsafe {

namespace vault {

struct StructuredDataUnresolvedEntryValue {
  StructuredDataUnresolvedEntryValue();
  StructuredDataUnresolvedEntryValue(const StructuredDataUnresolvedEntryValue& other);
  StructuredDataUnresolvedEntryValue& operator=(StructuredDataUnresolvedEntryValue other);
  // TODO(Team) when we get std::optional we can have move ctr but boost prevents this just now
  boost::optional<StructuredDataVersions::VersionName> version;
  boost::optional<StructuredDataVersions::VersionName> new_version;
  boost::optional<routing::ReplyFunctor> reply_functor;
  boost::optional<StructuredDataVersions::serialised_type> serialised_db_value;  // account xfer
};

// no move in boost::optional uncomment when std::optional is available
// void swap(const StructuredDataUnresolvedEntryValue& lhs,
//           const StructuredDataUnresolvedEntryValue& rhs) MAIDSAFE_NOEXCEPT;

bool operator==(const StructuredDataUnresolvedEntryValue& lhs,
                const StructuredDataUnresolvedEntryValue& rhs);

bool operator!=(const StructuredDataUnresolvedEntryValue& lhs,
                const StructuredDataUnresolvedEntryValue& rhs);

bool operator<(const StructuredDataUnresolvedEntryValue& lhs,
               const StructuredDataUnresolvedEntryValue& rhs);

bool operator>(const StructuredDataUnresolvedEntryValue& lhs,
               const StructuredDataUnresolvedEntryValue& rhs);

bool operator<=(const StructuredDataUnresolvedEntryValue& lhs,
                const StructuredDataUnresolvedEntryValue& rhs);

bool operator>=(const StructuredDataUnresolvedEntryValue& lhs,
                const StructuredDataUnresolvedEntryValue& rhs);

}  // namespace vault

}  // namespace maidsafe

#endif  // MAIDSAFE_VAULT_STRUCTURED_DATA_MANAGER_STRUCTURED_DATA_UNRESOLVED_ENTRY_VALUE_H_
